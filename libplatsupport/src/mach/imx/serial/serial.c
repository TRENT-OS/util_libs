/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <autoconf.h>
#include <platsupport/gen_config.h>

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include <string.h>

#include "../../../chardev.h"

#ifndef UART_REF_CLK
#error "UART_REF_CLK undefined"
#endif

#define UART_SR1_RRDY          BIT( 9)
#define UART_SR1_TRDY          BIT(13)
/* CR1 */
#define UART_CR1_UARTEN        BIT( 0)
#define UART_CR1_RRDYEN        BIT( 9)
/* CR2 */
#define UART_CR2_SRST          BIT( 0)
#define UART_CR2_RXEN          BIT( 1)
#define UART_CR2_TXEN          BIT( 2)
#define UART_CR2_ATEN          BIT( 3)
#define UART_CR2_RTSEN         BIT( 4)
#define UART_CR2_WS            BIT( 5)
#define UART_CR2_STPB          BIT( 6)
#define UART_CR2_PROE          BIT( 7)
#define UART_CR2_PREN          BIT( 8)
#define UART_CR2_RTEC          BIT( 9)
#define UART_CR2_ESCEN         BIT(11)
#define UART_CR2_CTS           BIT(12)
#define UART_CR2_CTSC          BIT(13)
#define UART_CR2_IRTS          BIT(14)
#define UART_CR2_ESCI          BIT(15)
/* CR3 */
#define UART_CR3_RXDMUXDEL     BIT( 2)
/* FCR */
#define UART_FCR_RFDIV(x)      ((x) * BIT(7))
#define UART_FCR_RFDIV_MASK    UART_FCR_RFDIV(0x7)
#define UART_FCR_RXTL(x)       ((x) * BIT(0))
#define UART_FCR_RXTL_MASK     UART_FCR_RXTL(0x1F)
/* SR2 */
#define UART_SR2_RXFIFO_RDR    BIT(0)
#define UART_SR2_TXFIFO_EMPTY  BIT(14)
/* RXD */
#define UART_URXD_READY_MASK   BIT(15)
#define UART_BYTE_MASK         0xFF

struct imx_uart_regs {
    uint32_t rxd;      /* 0x000 Receiver Register */
    uint32_t res0[15];
    uint32_t txd;      /* 0x040 Transmitter Register */
    uint32_t res1[15];
    uint32_t cr1;      /* 0x080 Control Register 1 */
    uint32_t cr2;      /* 0x084 Control Register 2 */
    uint32_t cr3;      /* 0x088 Control Register 3 */
    uint32_t cr4;      /* 0x08C Control Register 4 */
    uint32_t fcr;      /* 0x090 FIFO Control Register */
    uint32_t sr1;      /* 0x094 Status Register 1 */
    uint32_t sr2;      /* 0x098 Status Register 2 */
    uint32_t esc;      /* 0x09c Escape Character Register */
    uint32_t tim;      /* 0x0a0 Escape Timer Register */
    uint32_t bir;      /* 0x0a4 BRM Incremental Register */
    uint32_t bmr;      /* 0x0a8 BRM Modulator Register */
    uint32_t brc;      /* 0x0ac Baud Rate Counter Register */
    uint32_t onems;    /* 0x0b0 One Millisecond Register */
    uint32_t ts;       /* 0x0b4 Test Register */
};
typedef volatile struct imx_uart_regs imx_uart_regs_t;

static inline imx_uart_regs_t *imx_uart_get_priv(
    ps_chardevice_t *d)
{
    return (imx_uart_regs_t *)d->vaddr;
}

int uart_getchar(
    ps_chardevice_t *d)
{
    imx_uart_regs_t *regs = imx_uart_get_priv(d);
    uint32_t reg = 0;
    int c = -1;

    if (regs->sr2 & UART_SR2_RXFIFO_RDR) {
        reg = regs->rxd;
        if (reg & UART_URXD_READY_MASK) {
            c = reg & UART_BYTE_MASK;
        }
    }
    return c;
}

static int internal_is_tx_fifo_busy(
    imx_uart_regs_t *regs)
{
    /* check the TXFE (transmit buffer FIFO empty) flag, which is cleared
     * automatically when data is written to the TxFIFO. Even though the flag
     * is set, the actual data transmission via the UART's 32 byte FIFO buffer
     * might still be in progress.
     */
    return (0 == (regs->sr2 & UART_SR2_TXFIFO_EMPTY));
}

int uart_putchar(
    ps_chardevice_t *d,
    int c)
{
    imx_uart_regs_t *regs = imx_uart_get_priv(d);

    if (internal_is_tx_fifo_busy(regs)) {
        return -1;
    }

    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        /* write CR first */
        regs->txd = '\r';
        /* if we transform a '\n' (LF) into '\r\n' (CR+LF) this shall become an
         * atom, ie we don't want CR to be sent and then fail at sending LF
         * because the TX FIFO is full. Basically there are two options:
         *   - check if the FIFO can hold CR+LF and either send both or none
         *   - send CR, then block until the FIFO has space and send LF.
         * Assuming that if SERIAL_AUTO_CR is set, it's likely this is a serial
         * console for logging, so blocking seems acceptable in this special
         * case. The IMX6's TX FIFO size is 32 byte and TXFIFO_EMPTY is cleared
         * automatically as soon as data is written from regs->txd into the
         * FIFO. Thus the worst case blocking is roughly the time it takes to
         * send 1 byte to have room in the FIFO again. At 115200 baud with 8N1
         * this takes 10 bit-times, which is 10/115200 = 86,8 usec.
         */
        while (internal_is_tx_fifo_busy(regs)) {
            /* busy loop */
        }
    }

    regs->txd = c;
    return c;
}

static void uart_handle_irq(ps_chardevice_t *d UNUSED)
{
    /* TODO */
}

/*
 * BaudRate = RefFreq / (16 * (BMR + 1)/(BIR + 1) )
 * BMR and BIR are 16 bit
 */
static void imx_uart_set_baud(
    ps_chardevice_t *d,
    long bps)
{
    imx_uart_regs_t *regs = imx_uart_get_priv(d);
    uint32_t bmr, bir, fcr;
    fcr = regs->fcr;
    fcr &= ~UART_FCR_RFDIV_MASK;
    fcr |= UART_FCR_RFDIV(4);
    bir = 0xf;
    bmr = UART_REF_CLK / bps - 1;
    regs->bir = bir;
    regs->bmr = bmr;
    regs->fcr = fcr;
}

int serial_configure(
    ps_chardevice_t *d,
    long bps,
    int char_size,
    enum serial_parity parity,
    int stop_bits)
{
    imx_uart_regs_t *regs = imx_uart_get_priv(d);
    uint32_t cr2;
    /* Character size */
    cr2 = regs->cr2;
    if (char_size == 8) {
        cr2 |= UART_CR2_WS;
    } else if (char_size == 7) {
        cr2 &= ~UART_CR2_WS;
    } else {
        return -1;
    }
    /* Stop bits */
    if (stop_bits == 2) {
        cr2 |= UART_CR2_STPB;
    } else if (stop_bits == 1) {
        cr2 &= ~UART_CR2_STPB;
    } else {
        return -1;
    }
    /* Parity */
    if (parity == PARITY_NONE) {
        cr2 &= ~UART_CR2_PREN;
    } else if (parity == PARITY_ODD) {
        /* ODD */
        cr2 |= UART_CR2_PREN;
        cr2 |= UART_CR2_PROE;
    } else if (parity == PARITY_EVEN) {
        /* Even */
        cr2 |= UART_CR2_PREN;
        cr2 &= ~UART_CR2_PROE;
    } else {
        return -1;
    }
    /* Apply the changes */
    regs->cr2 = cr2;
    /* Now set the board rate */
    imx_uart_set_baud(d, bps);
    return 0;
}

static int imx6_uart_init(ps_chardevice_t *dev)
{
    assert(NULL != dev);

    imx_uart_regs_t *regs = imx_uart_get_priv(dev);

    /* Software reset */
    regs->cr2 &= ~UART_CR2_SRST;
    while (!(regs->cr2 & UART_CR2_SRST))
    {
        /* busy loop */
    }

    /* Line configuration */
    serial_configure(dev, 115200, 8, PARITY_NONE, 1);

    /* Enable the UART */
    regs->cr1 |= UART_CR1_UARTEN;                /* Enable the uart.                  */
    regs->cr2 |= UART_CR2_RXEN | UART_CR2_TXEN;  /* RX/TX enable                      */
    regs->cr2 |= UART_CR2_IRTS;                  /* Ignore RTS                        */
    regs->cr3 |= UART_CR3_RXDMUXDEL;             /* Configure the RX MUX              */
    /* Initialise the receiver interrupt.                                             */
    regs->cr1 &= ~UART_CR1_RRDYEN;               /* Disable recv interrupt.           */
    regs->fcr &= ~UART_FCR_RXTL_MASK;            /* Clear the rx trigger level value. */
    regs->fcr |= UART_FCR_RXTL(1);               /* Set the rx tigger level to 1.     */
    regs->cr1 |= UART_CR1_RRDYEN;                /* Enable recv interrupt.            */


#ifdef CONFIG_PLAT_IMX6
#include <platsupport/plat/mux.h>
    /* The UART1 on the IMX6 has the problem that the MUX is not correctly set,
     * and the RX PIN is not routed correctly.
     */
    if ((dev->id == IMX_UART1) && mux_sys_valid(&dev->ioops.mux_sys)) {
        if (mux_feature_enable(&dev->ioops.mux_sys, MUX_UART1, 0)) {
            /* Failed to configure the mux */
            ZF_LOGE("failed to configure MUX");
            return -1;
        }
    }
#endif

    return 0;
}

static void imx6_uart_dev_init(
    ps_chardevice_t *dev,
    const ps_io_ops_t *ops,
    enum chardev_id id,
    void *vaddr,
    const int *irqs)
{
    assert(NULL != dev);

    memset(dev, 0, sizeof(*dev));

    /* Set up all the  device properties. */
    dev->id         = id;
    dev->vaddr      = vaddr;
    dev->irqs       = irqs;

    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->ioops      = *ops;

    dev->flags      = SERIAL_AUTO_CR;
}

int uart_init(
    const struct dev_defn *defn,
    const ps_io_ops_t *ops,
    ps_chardevice_t *dev)
{
    assert(NULL != defn);
    assert(NULL != ops);
    assert(NULL != dev);

    /* Attempt to map the virtual address, assure this works */
    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        ZF_LOGE("chardev_map() failed");
        return -1;
    }

    imx6_uart_dev_init(dev, ops, defn->id, vaddr, defn->irqs);

    return imx6_uart_init(dev);
}

int uart_static_init(
    void *vaddr,
    const ps_io_ops_t *ops,
    ps_chardevice_t *dev)
{
    assert(NULL != vaddr);
    assert(NULL != ops);
    assert(NULL != dev);

    /* For now, the purpose of this function is to make this driver usable
     * from a CAmkES component, where the memory mapping and interrupt setup
     * has been done already in the CAmkES system configuration. Since we
     * currently use UART1 there, it's also hard-coded here. Ideally, we would
     * also pass the ID here besides just vaddr. If we can't change the
     * function signature, passing it in the "dev" parameter would also work.
     * One day the whole code should to be refactored to work smoother with
     * CAmkES or a stand-alone static setup.
     */
    imx6_uart_dev_init(dev, ops, IMX_UART1, vaddr, NULL);

    return imx6_uart_init(dev);

}

ps_chardevice_t*
ps_cdev_static_init(
    const ps_io_ops_t *ops,
    ps_chardevice_t* dev,
    void *params)
{
    assert(NULL != ops);
    assert(NULL != dev);

    if (params == NULL) {
        ZF_LOGE("params must no be NULL");
        return NULL;
    }

    void* vaddr = params;

    return uart_static_init(vaddr, ops, dev) ? NULL : dev;
}
