#include "system.h"
#include "hardware.h"
#include "MRF24J40MB_Driver.h"

////////////////////////////////////////////////////////////////////////

#define USE_HARDWARE_SPI

#define RADIO_ENABLE()              GPIO_reset(gpio_radio_cs)
#define RADIO_DISABLE()             GPIO_set(gpio_radio_cs)

#define RADIO_STACK_SIZE            512

#define RX_BUFFER_SIZE              1024
#define TX_TIME_LIST_LEN            5

// set this number away from 0x8000 to make it harder/easier to happen
#define RADIO_RAND_LIMIT            0x4000

#define RADIO_STATUS_TX_BUSY        0x0001
#define RADIO_STATUS_WAIT_ACK       0x0002
#define RADIO_STATUS_TX_FAIL        0x0004
#define RADIO_STATUS_RX_DONE        0x0008
#define RADIO_STATUS_BLOCK_TX       0x0010
#define RADIO_STATUS_THREAD_ON      0x0020

#define FRAME_CTRL_DATA             0x0001
#define FRAME_CTRL_COMMAND          0x0003
#define FRAME_CTRL_BEACON           0x0000

#define FRAME_CTRL_SECURITY         (1<<3)
#define FRAME_CTRL_ACK_PENDING      (1<<5)

#define FRAME_CTRL_SRC_ADDR_SHORT   (2<<10)
#define FRAME_CTRL_SRC_ADDR_LONG    (3<<10)

#define FRAME_CTRL_DEST_ADDR_SHORT  (2<<10)
#define FRAME_CTRL_DEST_ADDR_LONG   (3<<10)

////////////////////////////////////////////////////////////////////////

static handler_t gpio_radio_cs;
static handler_t gpio_radio_rst;
static uint8_t radio_curr_channel;
static uint32_t curr_ms_timer;

static uint16_t radio_status_mask;
static uint16_t radio_pan_id;
static uint16_t radio_addr;
static uint16_t last_seq_num;

static uint8_t radio_stack[ RADIO_STACK_SIZE ];
static thread_data_t radio_thread;

static uint8_t radio_tx_time_index;
static uint32_t radio_tx_time[TX_TIME_LIST_LEN];

static pipe_data_t rx_pipe_data;
static pipe_data_t tx_pipe_data;

static uint64_t radio_rand_seed;

static uint8_t radio_mac_address[8];

////////////////////////////////////////////////////////////////////////

void radio_set_long_addr( uint16_t address, uint8_t value )
{
    DI();
    RADIO_ENABLE();
    
    SPI_send_byte( SPI_RADIO, (((uint8_t)(address>>3))&0x7F)|0x80 );
    SPI_send_byte( SPI_RADIO, (((uint8_t)(address<<5))&0xE0)|0x10 );
    SPI_send_byte( SPI_RADIO, value );
    
    RADIO_DISABLE();
    EI();
}

void radio_set_short_addr( uint16_t address, uint8_t value )
{
    DI();
    RADIO_ENABLE();
    
    SPI_send_byte( SPI_RADIO, (uint8_t)(address&0xFF) );
    SPI_send_byte( SPI_RADIO, value );
    
    RADIO_DISABLE();
    EI();
}

uint8_t radio_get_long_addr( uint16_t address )
{
    uint8_t value;
    
    DI();
    RADIO_ENABLE();
    
    SPI_send_byte( SPI_RADIO, (((uint8_t)(address>>3))&0x7F)|0x80 );
    SPI_send_byte( SPI_RADIO, (((uint8_t)(address<<5))&0xE0) );
    value = SPI_read_byte( SPI_RADIO );
    
    RADIO_DISABLE();
    EI();
    
    return value;
}

uint8_t radio_get_short_addr( uint16_t address )
{
    uint8_t value;
    
    DI();
    RADIO_ENABLE();
    
    SPI_send_byte( SPI_RADIO, (uint8_t)(address&0xFF) );
    value = SPI_read_byte( SPI_RADIO );
    
    RADIO_DISABLE();
    EI();
    
    return value;
}

void radio_interrupt_handler( void )
{
    uint8_t status;
    uint16_t payload_size;
    uint16_t cnt;
    uint8_t * buf;
    
    DI();
    
    //read the interrupt status register to see what caused the interrupt
    status = radio_get_short_addr( READ_ISRSTS );
    
    if( status & ISRSTS_TXNIF )
    {
        radio_status_mask &= ~RADIO_STATUS_TX_BUSY;
        
        // register the tx time, read the ms timer from the thread, so we dont use syscall from interruption
        radio_tx_time[radio_tx_time_index] = curr_ms_timer - radio_tx_time[radio_tx_time_index];
        if( ++radio_tx_time_index >= TX_TIME_LIST_LEN ) radio_tx_time_index = 0;
        
        status = radio_get_short_addr( READ_TXSR );

        if( radio_status_mask & RADIO_STATUS_WAIT_ACK )
        {
            if( status & 0x01 )
            {
                radio_status_mask |= RADIO_STATUS_TX_FAIL;
            }
            
            radio_status_mask &= ~RADIO_STATUS_WAIT_ACK;
        }
    }
    else if( status & ISRSTS_RXIF )
    {
        //If the part is enabled for receiving packets right now
        //(not pending an ACK)
        //indicate that we have a packet in the buffer pending to 
        //be read into the buffer from the FIFO
        radio_set_short_addr(WRITE_BBREG1, 0x04);
        
        //get the size of the packet (m+n+2)
        //2 more bytes for RSSI and LQI reading 
        payload_size = radio_get_long_addr( 0x300 ) + 2;
        
        // check here the sequence number
        status = radio_get_long_addr( 0x303 );
        if( ( last_seq_num == 0xFFFF ) || ( ( last_seq_num & 0x0FF ) != status ) )
        {
            //copy all of the data from the FIFO into the RxBuffer
            buf = pipe_alloc_buffer( payload_size );
            for( cnt = 0; cnt < payload_size; cnt++ )
            {
                buf[cnt] = radio_get_long_addr( 0x301 + cnt );
            }

            pipe_write( &rx_pipe_data, buf, payload_size );
            radio_status_mask |= RADIO_STATUS_RX_DONE;
        }
        
        last_seq_num = status;

        // flush rx data
        radio_set_short_addr( WRITE_RXFLUSH, 0x01 );
        
        // enable radio to receive next packet
        radio_set_short_addr( WRITE_BBREG1, 0x00 );
    }
    
    EI();
}
    
void radio_main_process( void * args )
{
    uint8_t seq_num;
    uint8_t * buf;
    uint16_t payload;
    uint16_t frame_ctrl;
    uint16_t loc;
    uint16_t cnt;
    
    while( TRUE )
    {
        if( pipe_is_empty( &tx_pipe_data ) == FALSE )
        {
            // check if it's a blocking transmission
            if( ( ( ( radio_status_mask & RADIO_STATUS_BLOCK_TX ) == 0 ) ||
                  ( ( radio_status_mask & RADIO_STATUS_TX_BUSY ) == 0 ) ) &&
                ( rand_get_next( &radio_rand_seed ) > RADIO_RAND_LIMIT ) )
            {
                // incoming data to be transmitted
                buf = pipe_read( &tx_pipe_data, &payload );  
                loc = 0;
                seq_num = buf[0];
                payload--;
                buf++;
                
                frame_ctrl = FRAME_CTRL_DATA;
                frame_ctrl |= FRAME_CTRL_SRC_ADDR_SHORT;
                
                // header length = 7
                // 2 = frame control
                // 1 = frame sequence
                // 2 = PAN ID
                // 2 = dest address
                radio_set_long_addr( loc++, 7 );
                
                // header + payload
                radio_set_long_addr( loc++, 7 + payload );
                
                // frame control, LSB - MSB
                radio_set_long_addr( loc++, frame_ctrl & 0x0FF );
                radio_set_long_addr( loc++, frame_ctrl >> 8 );
                
                // sequence number
                radio_set_long_addr( loc++, seq_num );
                
                // PAN ID (LSB - MSB)
                radio_set_long_addr( loc++, radio_pan_id & 0x0FF );
                radio_set_long_addr( loc++, radio_pan_id >> 8 );

                // src address (LSB - MSB)
                radio_set_long_addr( loc++, radio_addr & 0x0FF );
                radio_set_long_addr( loc++, radio_addr >> 8 );
                
                // payload
                for( cnt = 0; cnt < payload; cnt++ )
                {
                    radio_set_long_addr( loc++, buf[cnt] );
                }
                
                // measure the tx time
                radio_tx_time[ radio_tx_time_index ] = timer_get_MS();
                
                // set flags
                radio_status_mask |= RADIO_STATUS_TX_BUSY;
                radio_status_mask &= ~RADIO_STATUS_TX_FAIL;
                radio_status_mask &= ~RADIO_STATUS_WAIT_ACK;
                
                // start tx
                radio_set_short_addr( WRITE_TXNMTRIG, 0x01 );
            }
        }
        
        thread_yield();
        
        // get the ms timer here to be used in the interrupt service
        curr_ms_timer = timer_get_MS();
        
        // change the seed to make the random number more random
        rand_get_next( &radio_rand_seed );
    }
}

////////////////////////////////////////////////////////////////////////////////

bool_t radio_init( uint16_t addr, uint8_t * mac, uint16_t pan )
{
    // this runs from init section
    __IO uint16_t cnt_delay;
    uint8_t cnt;
    
    uint8_t status;
    
    SPI_driver_init( SPI_RADIO );
    gpio_radio_cs = GPIO_get_port( GPIO_CS_RADIO );
    gpio_radio_rst = GPIO_get_port( GPIO_RESET_IO );
    
    GPIO_reset( gpio_radio_rst );
    
    // wait a little
    for( cnt_delay = 0; cnt_delay < 3000; cnt_delay++ ) {}
    
    GPIO_set( gpio_radio_rst );

    // wait a little
    for( cnt_delay = 0; cnt_delay < 3000; cnt_delay++ ) {}

    /* do a soft reset */
    cnt_delay = 0x8000;
    radio_set_short_addr( WRITE_SOFTRST, 0x07 );
    //do
    //{
    //    status = radio_get_short_addr(READ_SOFTRST);
    //    cnt_delay--;
    //}
    //while( ((status&0x07) != 0x00) && (cnt_delay > 0) );
    
    // if( cnt_delay == 0 ) return FALSE;
    
    // wait a little
    for( cnt_delay = 0; cnt_delay < 1000; cnt_delay++ ) {}

    /* flush the RX fifo */
    radio_set_short_addr(WRITE_RXFLUSH,0x01);

    /* Program the short MAC Address and PAN */
    radio_addr = addr;
    radio_pan_id = pan;
    radio_set_short_addr( WRITE_SADRL, addr & 0x0FF );
    radio_set_short_addr( WRITE_SADRH, addr >> 8 );
    radio_set_short_addr( WRITE_PANIDL, pan & 0x0FF );
    radio_set_short_addr( WRITE_PANIDH, pan >> 8 );  
    
    /* Program Long MAC Address*/
    for( cnt=0; cnt<8; cnt++ )
    {
        radio_set_short_addr( WRITE_EADR0 + ( cnt * 2 ), mac[cnt] );
        radio_mac_address[cnt] = mac[cnt];
    } 

    /* enable RF */
    radio_set_long_addr(RFCTRL2,0x80);    
    cnt_delay = radio_get_long_addr( RFCTRL2 );
    
    // MRF24J40 output power set to be -14.9dB
    // radio_set_long_addr(RFCTRL3, 0x70);
     
    // set as coordinator and only accept valid packets
    // radio_set_short_addr( WRITE_RXMCR, 0x0C );

    // set as coordinator and promiscuous mode
    radio_set_short_addr( WRITE_RXMCR, 0x0D );

    // MRF24J40 output power set to be -1.9dB
    radio_set_long_addr(RFCTRL3, 0x18);    
    
    /* program RSSI ADC with 2.5 MHz clock */
    radio_set_long_addr(RFCTRL6,0x90);

    /* set 100khz internal clock */
    radio_set_long_addr(RFCTRL7,0x80);

    /* enable VCO */
    radio_set_long_addr(RFCTRL8,0x10); 

    /* setup sleep clock control */
    radio_set_long_addr(SCLKDIV, 0x21);
    
    /* Program CCA mode using RSSI */
    radio_set_short_addr(WRITE_BBREG2,0x80);
    
    /* Enable the packet RSSI */
    radio_set_short_addr(WRITE_BBREG6,0x40);
    
    /* Program CCA, RSSI threshold values */
    radio_set_short_addr(WRITE_RSSITHCCA,0x60);    

    // enable auto GPIO for MRF24J40MB using LNA
    radio_set_long_addr(TESTMODE, 0x0F);
    
    // setup TX
    radio_set_short_addr(WRITE_FFOEN, 0x98);
    radio_set_short_addr(WRITE_TXPEMISP, 0x95);  
    
    // wait until the MRF24J40 in receive mode
    cnt_delay = 0x8000;
    do
    {
        status = radio_get_long_addr(RFSTATE);
        cnt_delay--;
    }
    while( ((status & 0xA0) != 0xA0) && (cnt_delay > 0) );
    
    if( cnt_delay == 0 ) return FALSE;
    
    // enable TX/RX interrupts
    radio_set_short_addr(WRITE_INTMSK,0xF6);

    #ifdef ENABLE_INDIRECT_MESSAGE
    PHYSetShortRAMAddr(WRITE_ACKTMOUT, 0xB9);
    #endif

    // Make RF communication stable under extreme temperatures
    radio_set_long_addr(RFCTRL0, 0x03);
    radio_set_long_addr(RFCTRL1, 0x02);

    // set default channel
    radio_curr_channel = 11;
    radio_set_channel(radio_curr_channel);

    // Define TURBO_MODE if more bandwidth is required
    // to enable radio to operate to TX/RX maximum 
    // 625Kbps
    #ifdef TURBO_MODE
    PHYSetShortRAMAddr(WRITE_BBREG0, 0x01);
    PHYSetShortRAMAddr(WRITE_BBREG3, 0x38);
    PHYSetShortRAMAddr(WRITE_BBREG4, 0x5C);

    PHYSetShortRAMAddr(WRITE_RFCTL,0x04);
    PHYSetShortRAMAddr(WRITE_RFCTL,0x00);
    #endif      

    GPIO_set_IRQ( radio_interrupt_handler, GPIO_IRQ_RF );

    // init network parameters
    last_seq_num = 0xFFFF;
    
    radio_tx_time_index = 0;
    memset( radio_tx_time, 0x00, sizeof( radio_tx_time ) );
    rand_get_seed( &radio_rand_seed );
    
    // by default wait until data is transmitted
    radio_status_mask = RADIO_STATUS_BLOCK_TX;
    
    // prepare pipes
    pipe_init( &rx_pipe_data, "MAC_PIPE_RX", 8 );
    pipe_init( &tx_pipe_data, "MAC_PIPE_TX", 8 );
    
    thread_create( &radio_thread, radio_main_process, NULL, radio_stack, RADIO_STACK_SIZE );
    
    return TRUE;
}

void radio_set_pan_id( uint16_t pan )
{
    radio_pan_id = pan;
    radio_set_short_addr( WRITE_PANIDL, pan & 0x0FF );
    radio_set_short_addr( WRITE_PANIDH, pan >> 8 );      
}

bool_t radio_is_tx_busy( void )
{
    if( radio_status_mask & RADIO_STATUS_TX_BUSY ) return TRUE;
    return FALSE;
}

void radio_enable_ack( bool_t state )
{
    if( state == TRUE )
    {
        radio_status_mask |= RADIO_STATUS_WAIT_ACK;
    }
    else
    {
        radio_status_mask &= ~RADIO_STATUS_WAIT_ACK;
    }
}

uint8_t radio_set_channel( uint8_t channel )
{
    if( ( channel >= 11 ) && ( channel < 26 ) )
    {
        radio_curr_channel = channel;
        radio_set_long_addr(RFCTRL0,((channel-11)<<4)|0x03);
        radio_set_short_addr(WRITE_RFCTL,0x04);
        radio_set_short_addr(WRITE_RFCTL,0x00);   
    }

    return radio_curr_channel;
}

void radio_set_power( uint8_t power )
{
    // power is in dB (0 ... -39)
    uint8_t reg;
    uint8_t temp_power;
    
    if( power > 39 ) power = 39;
    
    temp_power = power % 10;
    
    reg = ( power / 10 ) << 6;
    
    if( temp_power > 5 ) reg += 0x38;
    else if( temp_power > 4 ) reg += 0x30;
    else if( temp_power > 3 ) reg += 0x28;
    else if( temp_power > 2 ) reg += 0x20;
    else if( temp_power > 1 ) reg += 0x18;
    else if( temp_power > 0 ) reg += 0x10;
    
    radio_set_long_addr( RFCTRL3, reg );
}

bool_t radio_is_received_data( void )
{
    return ( pipe_get_buffer_size( &rx_pipe_data ) != 0 );
}

uint32_t radio_receive_data( uint8_t * buf, uint32_t size, rx_info_t * rx_info )
{
    uint32_t rx_data_len;
    uint8_t * buf_rx;
    
    rx_data_len = pipe_get_buffer_size( &rx_pipe_data );
    
    if( ( size == 0 ) || ( rx_data_len == 0 ) )
    {
        return rx_data_len;
    }
    else if( size < ( rx_data_len - 2 - 7 ) )
    {
        return 0;
    }
    
    buf_rx = pipe_read( &rx_pipe_data, NULL );
    
    if( rx_info != NULL )
    {
        // header length = 7
        // 2 = frame control
        // 1 = frame sequence
        // 2 = PAN ID
        // 2 = dest address
        rx_info->seq_num = buf_rx[2];
        rx_info->rssi = buf_rx[ rx_data_len - 2 ];
        rx_info->lqi = buf_rx[ rx_data_len - 1 ];
        
        memcpy( &rx_info->pan_id, &buf_rx[3], sizeof( uint16_t ) );
        memcpy( &rx_info->sender_id, &buf_rx[5], sizeof( uint16_t ) );
    }
    
    // copy only the payload
    memcpy( buf, &buf_rx[7], rx_data_len - 4 - 7 );
    pipe_release_buffer( buf_rx );
    
    return rx_data_len - 2 - 7;
}

uint32_t radio_transmit_data( uint8_t seq_num, uint8_t *buf, uint32_t size )
{
    uint8_t * buf_tx;
    
    // add the sequence at the beginning
    buf_tx = pipe_alloc_buffer( size + 1 );
    if( buf_tx == NULL ) return 0;
    
    buf_tx[0] = seq_num;
    memcpy( &buf_tx[1], buf, size );
    pipe_write( &tx_pipe_data, buf_tx, size+1 );
    
    return size;
}
