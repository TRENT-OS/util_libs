/*
 * Copyright 2019-2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>

#include "../../chardev.h"

/*
 *******************************************************************************
 * UART access primitives
 *******************************************************************************
 */

static uart_regs_t* uart_get_regs(
    ps_chardevice_t *d)
{
    return (uart_regs_t*)d->vaddr;
}

// static int internal_uart_is_rx_fifo_empty(
//     uart_regs_t* regs)
// {
//     // ToDo: implement me
//     return 0;
// }
//
// static int internal_uart_is_tx_fifo_full(
//     uart_regs_t* regs)
// {
//     // ToDo: implement me
//     return 0;
// }
//
// static int internal_uart_tx_byte(
//     uart_regs_t* regs,
//     uint8_t c)
// {
//     // ToDo: implement me
//     // regs->txdata = c
// }

/*
 *******************************************************************************
 * UART access API
 *******************************************************************************
 */

int uart_getchar(
    ps_chardevice_t *dev)
{
    // printf("uart_getchar");
    uart_regs_t* regs = uart_get_regs(dev);
    int c = -1;

    static uint8_t expected = 0;

    /* if UART is empty return an error */
    if (regs->lsr & UART_LSR_RX) {
        c = regs->rbr_dll_thr;
        if((uint8_t) c == expected)
            expected++;
        // else {
        //     printf("c = %x and expected = %x, lsr = %x\n", c, expected, regs->lsr);
        // }
    }

    // printf("uart_getchar with %d\n", c);
    return c;
}

void uart_do_putchar(
    ps_chardevice_t* dev,
    int c)
{
    uart_regs_t* regs = uart_get_regs(dev);

    while (0 == (regs->lsr & UART_LSR_TX))
    {
        // loop
    }

    regs->rbr_dll_thr = c;
}


int uart_putchar(
    ps_chardevice_t* dev,
    int c)
{
    // printf("uart_putchar() called\n");
    uart_do_putchar(dev,c);
    if (c == '\n') {
        uart_do_putchar(dev,'r');
    }

    // if (byte == '\n') {
    //     internal_uart_tx_byte(regs, '\r');
    //     /* If SERIAL_AUTO_CR is enabled, we assume this UART is used as a
    //      * console, so blocking is fine here.
    //      */
    //     while (internal_uart_is_tx_fifo_full(regs)) {
    //         /* busy waiting loop */
    //     }
    // }

    // internal_uart_tx_byte(regs, byte);

    return c;
}

static void uart_handle_irq(
    ps_chardevice_t* dev)
{
    // printf("uart_handle_irq() called\n");
    uart_regs_t* regs = uart_get_regs(dev);

    /* check for overrun error */
    if (regs->lsr & BIT(1)) {
         printf("overrun error detected\n");
         regs->lsr |= ~BIT(1);
    }
}

int uart_init(
    const struct dev_defn *defn,
    const ps_io_ops_t *ops,
    ps_chardevice_t *dev)
{
    void *vaddr = chardev_map(defn, ops);
    if (!vaddr) {
        return -1;
    }

    memset(dev, 0, sizeof(*dev));

    // Set up all the  device properties.
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    uart_regs_t* regs = uart_get_regs(dev);

    // ToDo: implement UART initialization

    /* Line configuration */
    // serial_configure(dev, 115200, 8, PARITY_NONE, 1);

    uint16_t clk_counter = 129; // on the MiG-V 1.0 Evaluation Board

    regs->lcr = 0x80; // DLAB=1 (baud rate divisor setup)
    regs->dlm_ier = (clk_counter >> 8) & 0xFF;
    regs->rbr_dll_thr = clk_counter & 0xFF;
    regs->iir_fcr = 0x27; // enables, resets and clears FIFO -> 1 IRQ per character
    regs->lcr = 0x03; // 8N1, DLAB=0
    regs->dlm_ier = (regs->dlm_ier & (~0xF)) | 0x1; // enable interrupts

    return 0;
}
