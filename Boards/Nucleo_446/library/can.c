#include "system.h"
#include "stm32f4xx.h"
#include "stm32f4xx_conf.h"
#include "command_hal.h"
#include "pipe.h"
#include "lists.h"
//#include "debug.h"

////////////////////////////////////////////////////////////////////

#define CAN_AT_500KBPS      12
#define CAN_AT_1MBPS        2
#define CAN_FILTER          0
#define CAN_MAX_WAIT        1024
#define MAX_DEV_FILTER      32
#define CAN_UNUSED          0xff
#define MAX_RX_FIFO         2
#define MAX_MAILBOX         3

#define HW_CAN1             0
#define HW_CAN2             1
#define HW_CAN_IN_USE       HW_CAN1

////////////////////////////////////////////////////////////////////

typedef struct can_bus_data_t_
{
    CAN_TypeDef *CANx;
    cmd_data_t   can_cmd;

    uint8_t      rx_irq0;
    uint8_t      rx_irq1;
    uint16_t     prescaler;
    uint8_t      filter;

    pin_data_t   tx_pin;
    pin_data_t   rx_pin;

} can_bus_t;

typedef struct can_device_data_t_
{
    list_node_t node;

    uint32_t    index;
    uint16_t    id;
    pipe_data_t pipe;

} can_device_data_t;

////////////////////////////////////////////////////////////////////

static const can_bus_t can_bus_data[] =
{
    {
        CAN1, { RCC_APB1PeriphClockCmd, RCC_APB1Periph_CAN1 },
        CAN1_RX0_IRQn,
        CAN1_RX1_IRQn,
        CAN_AT_500KBPS,
        CAN_FILTER,
        { GPIO_PinSource9, GPIO_AF_CAN1, GPIOB, { RCC_AHB1PeriphClockCmd, RCC_AHB1Periph_GPIOB } },
        { GPIO_PinSource8, GPIO_AF_CAN1, GPIOB, { RCC_AHB1PeriphClockCmd, RCC_AHB1Periph_GPIOB } },
    },
    {
        CAN2, { RCC_APB1PeriphClockCmd, RCC_APB1Periph_CAN2 },
        CAN2_RX0_IRQn,
        CAN2_RX1_IRQn,
        CAN_AT_500KBPS,
        CAN_FILTER,
        { GPIO_PinSource6, GPIO_AF_CAN2, GPIOB, { RCC_AHB1PeriphClockCmd, RCC_AHB1Periph_GPIOB } },
        { GPIO_PinSource5, GPIO_AF_CAN2, GPIOB, { RCC_AHB1PeriphClockCmd, RCC_AHB1Periph_GPIOB } },
    }
};

static list_data_t  device_list;
static pipe_data_t  tx_pipe;
static uint32_t     dev_index;
static bool_t       hw_ready;

////////////////////////////////////////////////////////////////////

static void can_tx_process( const can_bus_t * data )
{
    uint8_t mailbox;

    if( ( pipe_is_empty( &tx_pipe ) == FALSE ) )
    {
        if( ( mailbox = CAN_GetMailBox( data->CANx ) ) != CAN_TxStatus_NoMailBox )
        {
            CanTxMsg * tx_data = ( CanTxMsg * )pipe_read( &tx_pipe, NULL );
            CAN_Transmit( data->CANx, tx_data, mailbox );

            DEBUG_PRINTK( "CAN sending [0x%03x] on %d\r\n", tx_data->StdId, mailbox );
            pipe_release_buffer( ( uint8_t * )tx_data );
        }
    }
}

static void can_rx_irq( const can_bus_t * data, uint8_t fifo )
{
    while( CAN_MessagePending(data->CANx, fifo) >= 1 )
    {
        CanRxMsg rx_data;
        CAN_Receive(data->CANx, fifo, &rx_data);
        DEBUG_PRINTK( "CAN receiving [0x%03x] on %d\r\n", rx_data.StdId, fifo );

        can_device_data_t * dev = NULL;

        // find a destination
        while( ( dev = ( can_device_data_t * )list_get_prev( &device_list, dev ? &dev->node : NULL ) ) != NULL )
        {
            if( ( dev->id != 0 ) && ( dev->id == rx_data.StdId ) )
            {
                pipe_send_buffer( &dev->pipe, rx_data.Data, 8 );
            }
        }
    }
}

void CAN1_RX0_IRQHandler( void )
{
    can_rx_irq( &can_bus_data[HW_CAN1], CAN_FIFO0 );
}

void CAN1_RX1_IRQHandler( void )
{
    can_rx_irq( &can_bus_data[HW_CAN1], CAN_FIFO1 );
}

void CAN2_RX0_IRQHandler( void )
{
    can_rx_irq( &can_bus_data[HW_CAN2], CAN_FIFO0 );
}

void CAN2_RX1_IRQHandler( void )
{
    can_rx_irq( &can_bus_data[HW_CAN2], CAN_FIFO1 );
}

////////////////////////////////////////////////////////////////////

static void can_config( const can_bus_t * data )
{
    DEBUG_PRINTK( "can_config\r\n" );

    command_setup_irq( data->rx_irq0 );
    command_setup_irq( data->rx_irq1 );
    command_setup_gpio( &data->tx_pin );
    command_setup_gpio( &data->rx_pin );

    /* CAN configuration ********************************************************/
    /* Enable CAN clock */
    command_clock_enable( &data->can_cmd );

    /* CAN register init */
    CAN_DeInit(data->CANx);

    /* CAN cell init */
    CAN_InitTypeDef        CAN_InitStructure;
    CAN_InitStructure.CAN_TTCM = DISABLE;
    CAN_InitStructure.CAN_ABOM = DISABLE;
    CAN_InitStructure.CAN_AWUM = DISABLE;
    CAN_InitStructure.CAN_NART = DISABLE;
    CAN_InitStructure.CAN_RFLM = DISABLE;
    CAN_InitStructure.CAN_TXFP = DISABLE;
    CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
    CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;

    /* CAN Baudrate = 0.5 MBps (CAN clocked at 30 MHz) */
    CAN_InitStructure.CAN_BS1 = CAN_BS1_2tq;
    CAN_InitStructure.CAN_BS2 = CAN_BS2_4tq;
    CAN_InitStructure.CAN_Prescaler = data->prescaler;
    CAN_Init(data->CANx, &CAN_InitStructure);

    /* CAN filter init */
    CAN_FilterInitTypeDef  CAN_FilterInitStructure;
    CAN_FilterInitStructure.CAN_FilterNumber = 0;
    CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
    CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
    CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
    CAN_FilterInit(&CAN_FilterInitStructure);

    /* Enable FIFO message pending Interrupt */
    CAN_ITConfig(data->CANx, CAN_IT_FMP0, ENABLE);
    CAN_ITConfig(data->CANx, CAN_IT_FMP1, ENABLE);
}

///////////////////////////////////////////////////////////////////////////

static uint32_t add_device( uint16_t id )
{
    can_device_data_t * dev = ( can_device_data_t * )MMALLOC( sizeof( can_device_data_t ) );
    if( dev != NULL )
    {
        dev->id = id;
        dev->index = dev_index++;

        pipe_init( &dev->pipe, "PIPE_RX_CAN", MAX_PIPE_SIZE );
        list_insert_tail( &device_list, &dev->node );

        return dev->index;
    }

    return -1;
}

static can_device_data_t * find_device( uint32_t index )
{
    can_device_data_t * dev = NULL;

    while( ( dev = ( can_device_data_t * )list_get_prev( &device_list, dev ? &dev->node : NULL ) ) != NULL )
    {
        if( dev->index == index )
        {
            return dev;
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////

static void can_drv_init( uint32_t index )
{
    if( hw_ready == FALSE )
    {
        can_config( &can_bus_data[HW_CAN_IN_USE] );
        hw_ready = TRUE;
    }
}

static uint32_t can_drv_ioctl( uint32_t index, uint32_t param, void * value )
{
#if 0
    if( param == DEVICE_IS_DATA_AVAILABLE )
    {
        if( CAN_MessagePending(can_bus_data.CANx, CAN_FIFO0) >= 1 ) return CAN_FIFO0;
        else if( CAN_MessagePending(can_bus_data.CANx, CAN_FIFO1) >= 1 ) return CAN_FIFO1;
        else NEG_U32;
    }
    else if( ( param >= DEVICE_IS_TX_BUSY ) && ( param >= (DEVICE_IS_TX_BUSY+10) ) )
    {
        uint8_t mailbox = param - DEVICE_IS_TX_BUSY;
        return (CAN_TransmitStatus(can_bus_data.CANx, mailbox)  ==  CANTXOK);
    }
    else if( param == DEVICE_SET_RUNTIME_VALUE )
    {
        CanTxMsg TxMessage;
        uint8_t * buffer = ( uint8_t * )value;

        TxMessage.StdId = ( ( uint32_t )buffer[0] << 8) | buffer[1];
        TxMessage.ExtId = 0x01;
        TxMessage.RTR = CAN_RTR_DATA;
        TxMessage.IDE = CAN_ID_STD;
        TxMessage.DLC = 1;

        memcpy( TxMessage.Data, &buffer[2], TxMessage.Data );

        return CAN_Transmit( can_bus_data.CANx, &TxMessage );
    }
    else if( param == DEVICE_GET_RUNTIME_VALUE )
    {
        uint8_t * fifo = ( uint8_t * )value;
    }
#endif
    return 0;
}

static uint32_t can_read_buffer( uint32_t index, uint8_t * buffer, uint32_t size )
{
    can_device_data_t * dev = find_device(index);
    if( dev == NULL ) return 0;

    if( pipe_is_empty( &dev->pipe ) == TRUE ) return 0;

    if( buffer != NULL )
    {
        uint32_t pos = 0;
        while( size >= 8 )
        {
            uint8_t * temp = pipe_read( &dev->pipe, NULL );
            memcpy( &buffer[pos], temp, 8 );
            pos += 8;
            size -= 8;

            pipe_release_buffer( temp );
        }

        return pos;
    }
    else
    {
        return pipe_get_used( &dev->pipe ) * 8;
    }
}

static uint32_t can_write_buffer( uint32_t index, uint8_t * buffer, uint32_t size )
{
    if( ( size < 2 ) || ( size > 10 ) ) return 0;

    can_device_data_t * dev = find_device(index);
    if( dev == NULL ) return 0;

    CanTxMsg TxMessage;

    TxMessage.StdId = dev->id;
    TxMessage.ExtId = 0x00;
    TxMessage.RTR = CAN_RTR_Data;
    TxMessage.IDE = CAN_Id_Standard;
    TxMessage.DLC = 8;

    memset( TxMessage.Data, 0x00, sizeof( TxMessage.Data ) );
    memcpy( TxMessage.Data, buffer, size );

    pipe_send_buffer( &tx_pipe, ( uint8_t * )&TxMessage, sizeof( TxMessage ) );
    return size;
}

static void can_drv_close( uint32_t index )
{
    can_device_data_t * dev = find_device(index);
    if( dev != NULL )
    {
        list_remove( &device_list, &dev->node );
    }
}

static bool_t can_drv_validate( uint32_t index )
{
    if( index == 0 ) return TRUE;
    return FALSE;
}

static int can_drv_getindex( char * param )
{
    uint16_t id = tima_atoi(&param[1]);
    if( ( id == 0 ) || ( id > 0xFFF ) ) return -1;
    if( list_size( &device_list ) > 12 ) return -1;

    DEBUG_PRINTK( "Start CAN ID %d\n", id );

    return add_device( id );
}

///////////////////////////////////////////////////////////////////////////

static void device_delete_node( list_node_t * node )
{
    can_device_data_t * dev_data = ( can_device_data_t * )node;
    pipe_clear( &dev_data->pipe );
}

static void can_bus_driver_init( void )
{
    list_init( &device_list, device_delete_node );
    dev_index = 0;
    hw_ready = FALSE;
    pipe_init( &tx_pipe, "PIPE_TX_CAN", MAX_PIPE_SIZE );
}

static void device_process( void )
{
    can_tx_process( &can_bus_data[HW_CAN_IN_USE] );
    //can_rx_irq( &can_bus_data[HW_CAN1], CAN_FIFO0 );
    //can_rx_irq( &can_bus_data[HW_CAN1], CAN_FIFO1 );
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const device_data_t can_bus_device =
{
    "can?",
    &_in_use,

    can_drv_init,
    can_drv_close,
    NULL,
    can_drv_validate,
    can_drv_getindex,
    can_drv_ioctl,
    NULL,

    DEV_MODE_CHANNEL,

    can_read_buffer,
    can_write_buffer,

};

DECLARE_DEVICE_SECTION( can_bus_device );
DECLARE_INIT_DEVICE_SECTION( can_bus_driver_init );
DECLARE_PROCESS_SECTION( device_process );

