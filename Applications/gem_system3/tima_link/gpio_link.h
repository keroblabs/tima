#ifndef APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_GPIO_LINK_H_
#define APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_GPIO_LINK_H_

////////////////////////////////////////////////////////////////////

#include "types.h"
#include "message.h"

////////////////////////////////////////////////////////////////////

/*
 * GPIO_PExx = Linux system GPIO
 * GPIO_PGxx = fake GPIO (not physically connected, internal Linux control
 * GPIO_PHxx = Linux system GPIO mapped into GEM
 * GPIO_PIxx = GEM GPIO mapped into Linux system
 *
 */

#ifndef _USE_LINUX
typedef enum
{
    GPIO_PE00,  GPIO_PE01,  GPIO_PE02,  GPIO_PE03,  GPIO_PE04,  GPIO_PE05,  GPIO_PE06,  GPIO_PE07,
    GPIO_PE08,  GPIO_PE09,  GPIO_PE10,  GPIO_PE11,  GPIO_PI14,  GPIO_PI15,  GPIO_PI00,  GPIO_PI01,
    GPIO_PI02,  GPIO_PI03,  GPIO_PI10,  GPIO_PI11,  GPIO_PC03,  GPIO_PC07,  GPIO_PC16,  GPIO_PC17,
    GPIO_PC18,  GPIO_PC23,  GPIO_PC24,  GPIO_PB03,  GPIO_PB04,  GPIO_PB05,  GPIO_PB06,  GPIO_PB07,
    GPIO_PB08,  GPIO_PB10,  GPIO_PB11,  GPIO_PB12,  GPIO_PB13,  GPIO_PB14,  GPIO_PB15,  GPIO_PB16,
    GPIO_PB17,  GPIO_PH24,  GPIO_PH25,  GPIO_PH26,  GPIO_PH27,  GPIO_PH00,  GPIO_PH02,  GPIO_PH07,
    GPIO_PH09,  GPIO_PH10,  GPIO_PH11,  GPIO_PH12,  GPIO_PH13,  GPIO_PH14,  GPIO_PH15,  GPIO_PH16,
    GPIO_PH17,  GPIO_PH18,  GPIO_PH19,  GPIO_PH20,  GPIO_PH21,  GPIO_PH22,  GPIO_PH23,  GPIO_PG00,
    GPIO_PG01,  GPIO_PG02,  GPIO_PG03,  GPIO_PG04,  GPIO_PG05,  GPIO_PG06,  GPIO_PG07,  GPIO_PG08,
    GPIO_PG09,  GPIO_PG10,  GPIO_PG11,  GPIO_PI16,  GPIO_PI17,  GPIO_PI18,  GPIO_PI19,

    GPIO_MAX

} gpio_t;
#else
#include "gpio_driver.h"
#endif

////////////////////////////////////////////////////////////////////

#define MESSAGE_GPIO_CHANGED (Message_User_Defined + 0x3000)

////////////////////////////////////////////////////////////////////

void gpio_link_attach( gpio_t gpio, message_data_t * message_data );

int gpio_link_get( gpio_t gpio );

void gpio_link_set( gpio_t gpio, int state );
void gpio_link_init( void );

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_GPIO_LINK_H_ */
