/*
 * Copyright 2020, Hensoldt Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/util.h>

#include "../../chardev.h"


//------------------------------------------------------------------------------
static const int uart0_irqs[] = {MIGV_UART0_IRQ, -1};
static const int uart1_irqs[] = {MIGV_UART1_IRQ, -1};
static const int uart2_irqs[] = {MIGV_UART2_IRQ, -1};

#define MIGV_UART_DEFN(devid) {          \
    .id      = MIGV_UART##devid,    \
    .paddr   = MIGV_UART##devid##_PADDR, \
    .size    = BIT(12),             \
    .irqs    = uart##devid##_irqs,  \
    .init_fn = &uart_init           \
}

static const struct dev_defn dev_defn[] = {
    MIGV_UART_DEFN(0),
    MIGV_UART_DEFN(1),
    MIGV_UART_DEFN(2),
};


//------------------------------------------------------------------------------
struct ps_chardevice*
ps_cdev_init(enum chardev_id id,
             const ps_io_ops_t* ops,
             struct ps_chardevice* dev) {
    unsigned int i;
    for (i = 0; i < ARRAY_SIZE(dev_defn); i++) {
        if (dev_defn[i].id == id) {
            return (dev_defn[i].init_fn(dev_defn + i, ops, dev)) ? NULL : dev;
        }
    }
    return NULL;
}
