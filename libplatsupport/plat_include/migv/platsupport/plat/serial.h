/*
 * Copyright 2020-2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
#include <autoconf.h>

/* official device names */
enum chardev_id {
    MIGV_UART0,
    MIGV_UART1,
    MIGV_UART2,
    PS_SERIAL_DEFAULT = MIGV_UART1
};

#define MIGV_UART0_PADDR    0x00404000
#define MIGV_UART0_IRQ      9

#define MIGV_UART1_PADDR    0x00405000
#define MIGV_UART1_IRQ      10

#define MIGV_UART2_PADDR    0x00406000
#define MIGV_UART2_IRQ      11


#define DEFAULT_SERIAL_PADDR        MIGV_UART1_PADDR
#define DEFAULT_SERIAL_INTERRUPT    MIGV_UART1_IRQ

#define UART_LCR_DLAB    (1u << 7)   // DLAB bit in LCR reg
#define UART_IER_ERBFI   (1u << 0)   // ERBFI bit in IER reg
#define UART_IER_ETBEI   (1u << 1)   // ETBEI bit in IER reg

#define UART_LSR_RX      (1u << 0)   //UART_LSR_DR_MASK
#define UART_LSR_TX      (1u << 5)   // UART_LSR_TEMT_MASK


struct uart {
    uint32_t rbr_dll_thr; // 0x00 Receiver Buffer Register (Read Only)
                                   // Divisor Latch (LSB)
                                   // Transmitter Holding Register (Write Only)
    uint32_t dlm_ier;     // 0x04 Divisor Latch (MSB)
                                   // Interrupt Enable Register
    uint32_t iir_fcr;     // 0x08 Interrupt Identification Register (Read Only)
                                   // FIFO Control Register (Write Only)
    uint32_t lcr;         // 0xC Line Control Register
    uint32_t mcr;         // 0x10 MODEM Control Register
    uint32_t lsr;         // 0x14 Line Status Register
    uint32_t msr;         // 0x18 MODEM Status Register
    uint32_t scr;         // 0x1C Scratch Register
};
typedef volatile struct uart uart_regs_t;