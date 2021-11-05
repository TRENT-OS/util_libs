/*
 * Copyright (C) 2021, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/util.h>
#include <platsupport/gpio.h>
#include <platsupport/plat/gpio.h>

/*
 * According to 
 * 	- BCM2835 TRM, section 6.1 Register View
 * 	- BCM2711 TRM, section 5.2 Register View
 *
 * 000 = GPIO Pin is an input
 * 001 = GPIO Pin is an output
 * 100 = GPIO Pin takes alternate function 0
 * 101 = GPIO Pin takes alternate function 1
 * 110 = GPIO Pin takes alternate function 2
 * 111 = GPIO Pin takes alternate function 3
 * 011 = GPIO Pin takes alternate function 4
 * 010 = GPIO Pin takes alternate function 5
 */
enum {
    BCM283X_GPIO_FSEL_INPT = 0,
    BCM283X_GPIO_FSEL_OUTP = 1,
    BCM283X_GPIO_FSEL_ALT0 = 4,
    BCM283X_GPIO_FSEL_ALT1 = 5,
    BCM283X_GPIO_FSEL_ALT2 = 6,
    BCM283X_GPIO_FSEL_ALT3 = 7,
    BCM283X_GPIO_FSEL_ALT4 = 3,
    BCM283X_GPIO_FSEL_ALT5 = 2,
    BCM283X_GPIO_FSEL_MASK = 7
};

/**
 * Initialise the bcm283x GPIO system
 * 
 * @param[in] bankX      A virtual mapping for gpio bank X.
 * @param[out] gpio_sys  A handle to a gpio subsystem to populate.
 * @return               0 on success
 */
int bcm283x_gpio_sys_init(void *bank1, gpio_sys_t *gpio_sys);

/**
 * Sets the Function Select register for the given pin, which configures the pin
 * as Input, Output or one of the 6 alternate functions.
 * 
 * @param[in] pin   GPIO pin number
 * @param[in] mode  Mode to set the pin to, one of BCM2711_GPIO_FSEL_*
 * @return          0 on success
 */
int bcm283x_gpio_fsel(gpio_t *gpio, uint8_t mode);
