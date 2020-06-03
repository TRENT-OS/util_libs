/*
 * Copyright 2019, Hensoldt Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include <stdlib.h>
#include <string.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>

#include "../../chardev.h"

//------------------------------------------------------------------------------
static uart_regs_t*
uart_get_reg(ps_chardevice_t *dev)
{
    return (uart_regs_t*)dev->vaddr;
}


//------------------------------------------------------------------------------
static void
uart_handle_irq(ps_chardevice_t* dev)
{
    // ToDo: implement me, potentially noting here as IRQs are cleared when
    // the TX/RX watermark conditions are no longer met
}


//------------------------------------------------------------------------------
int
uart_putchar(ps_chardevice_t* dev, int c)
{
    // ToDo: implement me

    uart_regs_t* reg = uart_get_reg(dev);

    if ((c == '\n') && (dev->flags & SERIAL_AUTO_CR)) {
        // while isBusy;
        // write_reg('\r')
    }

    // while isBusy;
    // write_reg(c & 0xff)

    return 0;
}


//------------------------------------------------------------------------------
int
uart_getchar(ps_chardevice_t* dev)
{
    // ToDo: implement me

    uart_regs_t* reg = uart_get_reg(dev);

    // while isBusy;
    // return read_reg()

    return 0;
}


//------------------------------------------------------------------------------
int
uart_init(const struct dev_defn* defn,
          const ps_io_ops_t* ops,
          ps_chardevice_t* dev)
{
    void* vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    memset(dev, 0, sizeof(*dev));

    // Set up all the  device properties.
    dev->id         = defn->id;
    dev->vaddr      = (void*)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    uart_regs_t* reg = uart_get_reg(dev);

    // ToDo: implement UART initialization
    // reg->foo = bar

    return 0;
}
