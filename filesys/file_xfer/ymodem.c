#include "ymodem.h"
#include "system.h"
#include "commands.h"
#include "timer.h"
#include "efs.h"

///////////////////////////////////////////////////////////////

static uint8_t serial_buffer[PACKET_1K_SIZE+PACKET_OVERHEAD];
static char file_name[FILE_NAME_LENGTH];

///////////////////////////////////////////////////////////////

static void ymodem_SleepForMS( uint32_t period )
{
	timer_data_t sleep_ms;

	timer_Start( &sleep_ms, period );
	while( timer_Check( &sleep_ms ) == FALSE )
	{
		SYSTEM_EVENTS();
	}
}

static uint16_t crc16(const uint8_t *buf, uint32_t count)
{
	uint16_t crc = 0;
	int i;

	while(count--) {
		crc = crc ^ *buf++ << 8;

		for (i=0; i<8; i++) {
			if (crc & 0x8000) {
				crc = crc << 1 ^ 0x1021;
			} else {
				crc = crc << 1;
			}
		}
	}
	return crc;
}

static int _getchar( device_t * dev_tty, uint32_t timeout )
{
	timer_data_t rx_timeout;
	uint8_t rx_byte;

	timer_Start( &rx_timeout, timeout );

	while( timer_Check( &rx_timeout ) == FALSE )
	{
		if( device_is_rx_pending( dev_tty ) > 0 )
		{
			device_read_buffer( dev_tty, &rx_byte, 1 );
			return rx_byte;
		}

        SYSTEM_EVENTS();
	}

	return -1;
}

static void _putchar( device_t * dev_tty, int data )
{
	uint8_t data8 = ( uint8_t )data;
	device_write_buffer( dev_tty, &data8, 1 );
}

static const char *u32_to_str(uint32_t val)
{
	/* Maximum number of decimal digits in u32 is 10 */
	static char num_str[11];
	int  pos = 10;
	num_str[10] = 0;

	if (val == 0) {
		/* If already zero then just return zero */
		return "0";
	}

	while ((val != 0) && (pos > 0)) {
		num_str[--pos] = (val % 10) + '0';
		val /= 10;
	}

	return &num_str[pos];
}

static uint32_t str_to_u32(char* str)
{
	const char *s = str;
	uint32_t acc;
	int c;

	/* strip leading spaces if any */
	do {
		c = *s++;
	} while (c == ' ');

	for (acc = 0; (c >= '0') && (c <= '9'); c = *s++) {
		c -= '0';
		acc *= 10;
		acc += c;
	}
	return acc;
}

/* Returns 0 on success, 1 on corrupt packet, -1 on error (timeout): */
static int receive_packet(device_t * dev_tty, char *data, int *length)
{
	int i, c;
	uint32_t packet_size;

	*length = 0;

	c = _getchar(dev_tty, PACKET_TIMEOUT);
	if (c < 0) {
		return -1;
	}

	switch(c) {
	case SOH:
		packet_size = PACKET_SIZE;
		break;
	case STX:
		packet_size = PACKET_1K_SIZE;
		break;
	case EOT:
		return 0;
	case CAN:
		c = _getchar(dev_tty, PACKET_TIMEOUT);
		if (c == CAN) {
			*length = -1;
			return 0;
		}
	default:
		/* This case could be the result of corruption on the first octet
		* of the packet, but it's more likely that it's the user banging
		* on the terminal trying to abort a transfer. Technically, the
		* former case deserves a NAK, but for now we'll just treat this
		* as an abort case.
		*/
		*length = -1;
		return 0;
	}

	*data = (char)c;

	for(i = 1; i < (int)(packet_size + PACKET_OVERHEAD); ++i) 
	{
		c = _getchar(dev_tty, PACKET_TIMEOUT);
		if (c < 0) 
		{
			return -1;
		}
		data[i] = (char)c;
	}

	/* Just a sanity check on the sequence number/complement value.
	 * Caller should check for in-order arrival.
	 */
	if ((uint8_t)data[PACKET_SEQNO_INDEX] != (uint8_t)((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff)) 
	{
		return 1;
	}

	if (crc16((const uint8_t*)data + PACKET_HEADER, packet_size + PACKET_TRAILER) != 0) 
	{
		return 1;
	}
	*length = packet_size;

	return 0;
}

/* Returns the length of the file received, or 0 on error: */
uint32_t ymodem_receive( device_t * dev_tty, char * filename, char * curr_dir, uint32_t max_len )
{
	//uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
	bool_t recv_on;
	int packet_length, i, file_done, session_done, crc_tries, crc_nak;
	uint32_t packets_received, errors, first_try = 1;
	char file_size[FILE_SIZE_LENGTH+1], *file_ptr;
	//uint8_t *buf_ptr;
	int size = 0, ret;
	int total_size = 0;
	File * fp_get_file = NULL;

	strcpy( file_name, curr_dir );
	strcat( file_name, "/" );

	recv_on = FALSE;

	// printf("Ymodem rcv:\n");
	//file_name[0] = 0;

	for (session_done = 0, errors = 0; ; ) {
		crc_tries = crc_nak = 1;
		if (!first_try) {
			_putchar(dev_tty, CRC);
		}
		first_try = 0;
		packets_received = 0;
		file_done = 0;

		//for (packets_received = 0, file_done = 0; ; ) 
		while(!session_done)
		{
			ret = receive_packet(dev_tty, (char * )serial_buffer, &packet_length);
			//DEBUG_PRINTK( "done [%d]\r\n", packets_received );
			switch (ret) 
			{

			case 0:
				errors = 0;
				switch (packet_length) {
					case -1:  
						if( recv_on == TRUE )
						{
							/* abort */
							_putchar(dev_tty, ACK);
							return 0;
						}
						break;
					case 0:   /* end of transmission */
						_putchar(dev_tty, ACK);
						/* Should add some sort of sanity check on the number of
						 * packets received and the advertised file length.
						 */
						file_done = 1;
						break;
					default:  /* normal packet */
						recv_on = TRUE;
					if ((serial_buffer[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff)) {
						_putchar(dev_tty, NAK);
					} else {
						if (packets_received == 0) {
							/* The spec suggests that the whole data section should
							 * be zeroed, but I don't think all senders do this. If
							 * we have a NULL filename and the first few digits of
							 * the file length are zero, we'll call it empty.
							 */
							for (i = PACKET_HEADER; i < PACKET_HEADER + 4; i++) {
								if (serial_buffer[i] != 0) {
									break;
								}
							}
							if (i < PACKET_HEADER + 4) {  /* filename packet has data */
								strcat( file_name, (char * )serial_buffer + PACKET_HEADER );
								strcpy( filename, (char * )serial_buffer + PACKET_HEADER );
								file_ptr = strchr( (char * )serial_buffer + PACKET_HEADER, 0 );

								for (++file_ptr, i = 0; *file_ptr != ' ' && i < FILE_SIZE_LENGTH; ) {
									file_size[i++] = *file_ptr++;
								}
								file_size[i++] = '\0';
								total_size = size = (int)str_to_u32(file_size);

								//DEBUG_PRINTK( "Ready to receive %s, %d bytes\r\n", file_name, size );

								fp_get_file = file_fopen( file_name, FS_MODE_WRITE );
								if( fp_get_file == NULL )
								{
									_putchar(dev_tty, ACK);
									file_done = 1;
									session_done = 1;
									break;
								}

								_putchar(dev_tty, ACK);
								_putchar(dev_tty, crc_nak ? CRC : NAK);
								crc_nak = 0;
							} else {  /* filename packet is empty; end session */
								_putchar(dev_tty, ACK);
								file_done = 1;
								session_done = 1;
								break;
							}
						} else {
							/* This shouldn't happen, but we check anyway in case the
							 * sender lied in its filename packet:
							 */
							if (size <= 0 ) 
							{
								_putchar(dev_tty, CAN);
								_putchar(dev_tty, CAN);
								return 0;
							}

							if( size < packet_length ) packet_length = size;
							file_fwrite( &serial_buffer[PACKET_HEADER], packet_length, 1, fp_get_file );

							size -= packet_length;
							_putchar(dev_tty, ACK);
						}
						++packets_received;
					}  /* sequence number ok */
				}
				break;

			default:
				if (packets_received != 0) {
					if (++errors >= MAX_ERRORS) {
						_putchar(dev_tty, CAN);
						_putchar(dev_tty, CAN);
						return 0;
					}
				}
				_putchar(dev_tty, CRC);
			}
			if (file_done && (!session_done)) 
			{
				file_fclose( fp_get_file );
				break;
			}
		}  /* receive packets */

		if (session_done)
			break;

	}  /* receive files */

	return total_size;
}

static void send_packet(device_t * dev_tty, uint8_t *data, int block_no)
{
	int crc, packet_size;

	/* We use a short packet for block 0 - all others are 1K */
	if (block_no == 0) 
	{
		packet_size = PACKET_SIZE;
	} 
	else 
	{
		packet_size = PACKET_1K_SIZE;
	}

	crc = crc16(data, packet_size);
	/* 128 byte packets use SOH, 1K use STX */
	_putchar(dev_tty, (block_no==0)?SOH:STX);
	_putchar(dev_tty, block_no & 0xFF);
	_putchar(dev_tty, ~block_no & 0xFF);

	device_write_buffer( dev_tty, data, packet_size );

	_putchar(dev_tty, (crc >> 8) & 0xFF);
	_putchar(dev_tty, crc & 0xFF);
}

/* Send block 0 (the filename block). filename might be truncated to fit. */
static void send_packet0(device_t * dev_tty, char* filename, uint32_t size)
{
	uint32_t count = 0;
	uint8_t block[PACKET_SIZE];
	const char* num;
	char * name;
	char * delim;

	if( filename != NULL )
	{
		name = filename;

		while( ( delim = strchr( name, '/' ) ) != NULL )
		{
			name = delim + 1;
		}

		if (name) {
			while (*name && (count < PACKET_SIZE-FILE_SIZE_LENGTH-2)) {
				block[count++] = *name++;
			}
			block[count++] = 0;

			num = u32_to_str(size);
			while(*num) {
				block[count++] = *num++;
			}
		}
	}

	while (count < PACKET_SIZE) {
		block[count++] = 0;
	}

	send_packet(dev_tty, block, 0);
}


static void send_data_packets(device_t * dev_tty, File * fp, uint32_t size)
{
	int blockno = 1;
	uint32_t send_size;
	int ch;

	while (!file_feof(fp)) {
		memset( serial_buffer, 0x00, sizeof( serial_buffer ) );
		send_size = file_fread( serial_buffer, PACKET_1K_SIZE, 1, fp );

		do {
			send_packet(dev_tty, serial_buffer, blockno);
			ch = _getchar(dev_tty, PACKET_TIMEOUT);
		}
		while( ch == NAK );

		if (ch == ACK)  {
			blockno++;
		} else {
			if((ch == CAN) || (ch == -1)) {
				return;
			}
		}
	}

	do {
		_putchar(dev_tty, EOT);
		ch = _getchar(dev_tty, PACKET_TIMEOUT);
	} 
	while((ch != ACK) && (ch != -1));

	/* Send last data packet */
	if (ch == ACK) 
	{
		ch = _getchar(dev_tty, PACKET_TIMEOUT);
		if (ch == CRC) {
			do {
				send_packet0(dev_tty, NULL, 0);
				ch = _getchar(dev_tty, PACKET_TIMEOUT);
			} 
			while((ch != ACK) && (ch != -1));
		}
	}
}

uint32_t ymodem_send_file( device_t * dev_tty, char * filename )
{
	File * fp_send_file;
	uint32_t size;
	int ch, crc_nak;

	crc_nak = 1;

	fp_send_file = file_fopen( filename, FS_MODE_READ );
	if( fp_send_file == NULL ) return 0;

	// flush RX buffer
	ymodem_SleepForMS( 500 );
	while( device_read_buffer( dev_tty, serial_buffer, PACKET_1K_SIZE ) )
	{
	}

	size = file_fioctrl( fp_send_file, IO_CONTROL_FILE_SIZE, NULL );

	while(1)
	{
		send_packet0(dev_tty, filename, size);

		ch = _getchar(dev_tty, PACKET_TIMEOUT);

		if( ch == ACK )
		{
			ch = _getchar(dev_tty, PACKET_TIMEOUT);
			if( ch == CRC )
			{
				send_data_packets( dev_tty, fp_send_file, size );
				file_fclose( fp_send_file );
				return size;
			} 
			else if ((ch == CRC) && (crc_nak)) 
			{
				crc_nak = 0;
				continue;
			} 
			else if ((ch != NAK) || (crc_nak)) 
			{
				break;
			}
		}
	}

	_putchar(dev_tty, CAN);
	_putchar(dev_tty, CAN);

	// flush RX buffer
	ymodem_SleepForMS( 500 );
	while( device_read_buffer( dev_tty, serial_buffer, PACKET_1K_SIZE ) )
	{
	}

	return 0;
}
