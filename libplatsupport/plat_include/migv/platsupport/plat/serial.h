/*
 * Copyright 2020, Hensoldt Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#pragma once
#include <autoconf.h>

/* official device names */
enum chardev_id {
    MIGV_UART0,
    MIGV_UART1,
    MIGV_UART2,
    PS_SERIAL_DEFAULT = MIGV_UART0
};

#define MIGV_UART0_PADDR    0x00404000
#define MIGV_UART0_IRQ      9

#define MIGV_UART1_PADDR    0x00405000
#define MIGV_UART1_IRQ      10

#define MIGV_UART2_PADDR    0x00406000
#define MIGV_UART2_IRQ      11


#define DEFAULT_SERIAL_PADDR        MIGV_UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT    MIGV_UART0_IRQ


typedef struct {
    // ToDo: add regs

    // volatile uint32_t reg1:
    // volatile uint32_t reg1:
    // ...

} uart_regs_t;
