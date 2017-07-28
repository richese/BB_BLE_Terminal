/* ----------------------------------------------------------------------------
 * Copyright (c) 2016 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * This code is the property of ON Semiconductor and may not be redistributed
 * in any form without prior written permission from ON Semiconductor.
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 * ----------------------------------------------------------------------------
 * gpio_util.c
 * - GPIO pin utility functions
 * ----------------------------------------------------------------------------
 * $Revision: 1.6 $
 * $Date: 2016/10/20 20:37:43 $
 * ------------------------------------------------------------------------- */

#include <rsl10.h>
#include "../include/gpio_util.h"


/* ----------------------------------------------------------------------------
 * Function      : void Buzz_GPIOs_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Configure a number of GPIO pins for testing purposes
 *                 for testing purposes
 * Inputs        : None
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Buzz_GPIOs_Init(void)
{
    /* Set mode for DIO pins uses for testing purposes */
    Sys_DIO_Config(APP_INIT_PIN, DIO_MODE_GPIO_OUT_0);
    Sys_DIO_Config(SVC_NOTIF_PIN, DIO_MODE_GPIO_OUT_0);
    Sys_DIO_Config(TIMER0_ISR_PIN, DIO_MODE_GPIO_OUT_0);
    Sys_DIO_Config(7, DIO_MODE_GPIO_OUT_0);

    /* Clear DIO pins uses for testing purposes */
    DIO->DATA &= ~((uint32_t)((0x1U << APP_INIT_PIN)  |
                              (0x1U << SVC_NOTIF_PIN) |
                              (0x1U << TIMER0_ISR_PIN)|
                              (0x1U << 7)));
}
/* ----------------------------------------------------------------------------
 * Function      : void Buzz_GPIO(uint32_t gpio_pin, unsigned int toggle_num,
 *                                unsigned int toggle_duration)
 * ----------------------------------------------------------------------------
 * Description   : Send a buzz to a specific GPIO pin for testing purposes
 * Inputs        : - gpio_pin          - GPIO pin
 *                 - toggle_num        - Number of toggles
 *                 - toggle_duration   - Duration of each toggle [usec]
 * Outputs       : None
 * Assumptions   : System clock of 8 MHz
 * ------------------------------------------------------------------------- */
void Buzz_GPIO(uint32_t gpio_pin, unsigned int toggle_num,
               unsigned int toggle_duration)
{
    unsigned int i;

    Sys_GPIO_Set_Low(gpio_pin);
    for (i = 0; i < toggle_num; i++)
    {
        Sys_Watchdog_Refresh();

        /* ~toggle_duration usec delay required:
         * Assumes system clock of 8 MHz, then the number of cycles is
         * calculated as follows: toggle_duration [usec] * 8 * 10^6 [cycle/sec]
         *                        = toggle_duration * 8 [cycle] */
        Sys_Delay_ProgramROM((uint32_t)(toggle_duration * 8));
        Sys_GPIO_Toggle(gpio_pin);
    }
    Sys_Watchdog_Refresh();
    Sys_GPIO_Set_Low(gpio_pin);
}
/* ----------------------------------------------------------------------------
 * Function      : void Test_GPIO8_Init(void)
 * ----------------------------------------------------------------------------
 * Description   : Configure a number of GPIO pins for testing purposes
 *                 for testing purposes
 * Inputs        : None
 * Outputs       : Pin 8 to High
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Test_GPIO8_Init(void)
{
	Sys_DIO_Config(10, DIO_MODE_GPIO_OUT_0);
	Sys_DIO_Config(7, DIO_MODE_GPIO_OUT_0);
	// Output set to 0
	DIO->DATA &= ~((uint32_t)((0x1U << 10) | (0x1U << 7)));
	//Sys_GPIO_Toggle(10);
	Sys_GPIO_Set_Low(7);
	Sys_GPIO_Set_High(10);
}

void GPIO_4_5_to_Input(void){
	Sys_DIO_Config(4,DIO_MODE_GPIO_IN_0);
	Sys_DIO_Config(5,DIO_MODE_GPIO_IN_0);
}
