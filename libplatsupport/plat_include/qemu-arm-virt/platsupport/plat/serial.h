/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define UART0_PADDR  0x9000000
#define UART1_PADDR  0x9040000
#define UART2_PADDR  0x9100000
#define UART3_PADDR  0x9110000
#define UART4_PADDR  0x9120000

#define UART0_IRQ    33
#define UART1_IRQ    40
#define UART2_IRQ    44
#define UART3_IRQ    45
#define UART4_IRQ    46

enum chardev_id {
    PL011_UART0,
    PL011_UART1,
    PL011_UART2,
    PL011_UART3,
    PL011_UART4,
    /* Aliases */
    PS_SERIAL0 = PL011_UART0,
    PS_SERIAL1 = PL011_UART1,
    PS_SERIAL2 = PL011_UART2,
    PS_SERIAL3 = PL011_UART3,
    PS_SERIAL4 = PL011_UART4,
    /* defaults */
    PS_SERIAL_DEFAULT = PL011_UART0
};

#define DEFAULT_SERIAL_PADDR        UART0_PADDR
#define DEFAULT_SERIAL_INTERRUPT    UART0_IRQ

