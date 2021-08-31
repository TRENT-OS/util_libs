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

/*
 *******************************************************************************
 * UART access API
 *******************************************************************************
 */

int uart_getchar(
    ps_chardevice_t *dev)
{
    uart_regs_t* regs = uart_get_regs(dev);

    /* if UART is empty return an error */
    if (!(regs->lsr & UART_LSR_RX))
        return -1;

    return (uint8_t)(regs->rbr_dll_thr & 0xFF);
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
    uart_do_putchar(dev,c);
    if (c == '\n') {
        uart_do_putchar(dev,'r');
    }

    return c;
}

static void uart_handle_irq(
    ps_chardevice_t* dev)
{
    // nothing to do here
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

    uint16_t clk_counter = 195; // limit UART to baud rate of 76800 on the MiG-V 1.0 Evaluation Board
    regs->lcr = 0x80; // DLAB=1 (baud rate divisor setup)
    // clock configuration
    regs->dlm_ier = (clk_counter >> 8) & 0xFF; // divisor latch MSB
    regs->rbr_dll_thr = clk_counter & 0xFF; // divisor latch LSB
    // enables FIFO, resets RX and TX FIFO, enables 64 byte FIFO and
    // sets receiver trigger to 32 bytes (minimum level for success in test was 16 bytes)
    regs->iir_fcr = 0xA7;
    // set communication parameters
    regs->lcr = 0x3; // 8N1, DLAB=0
     // enable interrupts (only receive interrupt, as transmit interrupt currently seems to block)
    regs->dlm_ier = (regs->dlm_ier & (~0xF)) | 0x1;

    return 0;
}
