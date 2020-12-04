/*
 * Copyright 2017, DornerWorks
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <platsupport/driver_module.h>
#include <platsupport/fdt.h>
#include <ethdrivers/gen_config.h>
#include <ethdrivers/raw.h>
#include <ethdrivers/helpers.h>
#include <string.h>
#include <utils/util.h>
#include "enet.h"
#include "ocotp_ctrl.h"
#include "uboot/fec_mxc.h"
#include "uboot/miiphy.h"
#include "uboot/mx6qsabrelite.h"
#include "uboot/micrel.h"
#include "unimplemented.h"

#if defined(CONFIG_PLAT_IMX6)

#define IMX6_ENET_PADDR 0x02188000
#define IMX6_ENET_SIZE  0x00004000

#elif defined(CONFIG_PLAT_IMX8MQ_EVK)

#define IMX6_ENET_PADDR 0x30be0000
#define IMX6_ENET_SIZE  0x10000

#endif


#define IRQ_MASK    (NETIRQ_RXF | NETIRQ_TXF | NETIRQ_EBERR)

#define BUF_SIZE MAX_PKT_SIZE
#define DMA_ALIGN 32

struct descriptor {
    /* NOTE: little endian packing: len before stat */
#if BYTE_ORDER == LITTLE_ENDIAN
    uint16_t len;
    uint16_t stat;
#elif BYTE_ORDER == BIG_ENDIAN
    uint16_t stat;
    uint16_t len;
#else
#error Could not determine endianess
#endif
    uint32_t phys;
};

struct imx6_eth_data {
    struct enet *enet;
    uintptr_t tx_ring_phys;
    uintptr_t rx_ring_phys;
    volatile struct descriptor *tx_ring;
    volatile struct descriptor *rx_ring;
    unsigned int rx_size;
    unsigned int tx_size;
    void **rx_cookies;                   // Array (of rx_size elements) of type 'void *'
    unsigned int rx_remain;
    unsigned int tx_remain;
    void **tx_cookies;
    unsigned int *tx_lengths;
    /* track where the head and tail of the queues are for
     * enqueueing buffers / checking for completions */
    unsigned int rdt, rdh, tdt, tdh;
};

/* Receive descriptor status */
#define RXD_EMPTY     BIT(15) /* Buffer has no data. Waiting for reception. */
#define RXD_OWN0      BIT(14) /* Receive software ownership. R/W by user */
#define RXD_WRAP      BIT(13) /* Next buffer is found in ENET_RDSR */
#define RXD_OWN1      BIT(12) /* Receive software ownership. R/W by user */
#define RXD_LAST      BIT(11) /* Last buffer in frame. Written by the uDMA. */
#define RXD_MISS      BIT( 8) /* Frame does not match MAC (promiscuous mode) */
#define RXD_BROADCAST BIT( 7) /* frame is a broadcast frame */
#define RXD_MULTICAST BIT( 6) /* frame is a multicast frame */
#define RXD_BADLEN    BIT( 5) /* Incoming frame was larger than RCR[MAX_FL] */
#define RXD_BADALIGN  BIT( 4) /* Frame length does not align to a byte */
#define RXD_CRCERR    BIT( 2) /* The frame has a CRC error */
#define RXD_OVERRUN   BIT( 1) /* FIFO overrun */
#define RXD_TRUNC     BIT( 0) /* Receive frame > TRUNC_FL */

#define RXD_ERROR    (RXD_BADLEN  | RXD_BADALIGN | RXD_CRCERR |\
                      RXD_OVERRUN | RXD_TRUNC)

/* Transmit descriptor status */
#define TXD_READY     BIT(15) /* buffer in use waiting to be transmitted */
#define TXD_OWN0      BIT(14) /* Receive software ownership. R/W by user */
#define TXD_WRAP      BIT(13) /* Next buffer is found in ENET_TDSR */
#define TXD_OWN1      BIT(12) /* Receive software ownership. R/W by user */
#define TXD_LAST      BIT(11) /* Last buffer in frame. Written by the uDMA. */
#define TXD_ADDCRC    BIT(10) /* Append a CRC to the end of the frame */
#define TXD_ADDBADCRC BIT( 9) /* Append a bad CRC to the end of the frame */


/*----------------------------------------------------------------------------*/
struct imx6_eth_data *
get_dev_from_driver(
    struct eth_driver *driver)
{
    assert(driver);
    return (struct imx6_eth_data *)driver->eth_data;
}

/*----------------------------------------------------------------------------*/
struct enet *
get_enet_from_driver(
    struct eth_driver *driver)
{
    assert(driver);
    struct imx6_eth_data *dev = get_dev_from_driver(driver);
    assert(dev);
    return dev->enet;
}

/*----------------------------------------------------------------------------*/
static void
low_level_init(
    struct eth_driver *driver,
    uint8_t *mac,
    int *mtu)
{
    struct enet *enet = get_enet_from_driver(driver);
    enet_get_mac(enet, mac);
    *mtu = MAX_PKT_SIZE;
}

/*----------------------------------------------------------------------------*/
static void
fill_rx_bufs(
    struct eth_driver *driver)
{
    struct imx6_eth_data *dev = get_dev_from_driver(driver);
    __sync_synchronize();
    while (dev->rx_remain > 0) {
        /* request a buffer */
        void *cookie = NULL;
        int next_rdt = (dev->rdt + 1) % dev->rx_size;

        /* This fn ptr is either lwip_allocate_rx_buf or
         * lwip_pbuf_allocate_rx_buf (in src/lwip.c)
         */
        uintptr_t phys = driver->i_cb.allocate_rx_buf ?
                            driver->i_cb.allocate_rx_buf(
                                driver->cb_cookie,
                                BUF_SIZE,
                                &cookie)
                            : 0;
        if (!phys) {
            /* NOTE: This condition could happen if
             *       CONFIG_LIB_ETHDRIVER_NUM_PREALLOCATED_BUFFERS is less than
             *       CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT
             */
            break;
        }

        dev->rx_cookies[dev->rdt] = cookie;
        dev->rx_ring[dev->rdt].phys = phys;
        dev->rx_ring[dev->rdt].len = 0;

        __sync_synchronize();
        dev->rx_ring[dev->rdt].stat = RXD_EMPTY
                                      | (next_rdt == 0 ? RXD_WRAP : 0);
        dev->rdt = next_rdt;
        dev->rx_remain--;
    }
    __sync_synchronize();
    if (dev->rdt != dev->rdh && !enet_rx_enabled(dev->enet)) {
        enet_rx_enable(dev->enet);
    }
}

/*----------------------------------------------------------------------------*/
static void
free_desc_ring(
    struct imx6_eth_data *dev,
    ps_dma_man_t *dma_man)
{
    if (dev->rx_ring) {
        dma_unpin_free(
            dma_man,
            (void *)dev->rx_ring,
            sizeof(struct descriptor) * dev->rx_size);
        dev->rx_ring = NULL;
    }
    if (dev->tx_ring) {
        dma_unpin_free(
            dma_man,
            (void *)dev->tx_ring,
            sizeof(struct descriptor) * dev->tx_size);
        dev->tx_ring = NULL;
    }
    if (dev->rx_cookies) {
        free(dev->rx_cookies);
        dev->rx_cookies = NULL;
    }
    if (dev->tx_cookies) {
        free(dev->tx_cookies);
        dev->tx_cookies = NULL;
    }
    if (dev->tx_lengths) {
        free(dev->tx_lengths);
        dev->tx_lengths = NULL;
    }
}

/*----------------------------------------------------------------------------*/
static int
initialize_desc_ring(
    struct imx6_eth_data *dev,
    ps_dma_man_t *dma_man)
{
    dma_addr_t rx_ring = dma_alloc_pin(
                            dma_man,
                            sizeof(struct descriptor) * dev->rx_size,
                            0,
                            DMA_ALIGN);
    if (!rx_ring.phys) {
        LOG_ERROR("Failed to allocate rx_ring");
        return -1;
    }
    ps_dma_cache_clean_invalidate(
        dma_man,
        rx_ring.virt,
        sizeof(struct descriptor) * dev->rx_size);
    dev->rx_ring = rx_ring.virt;
    dev->rx_ring_phys = rx_ring.phys;

    dma_addr_t tx_ring = dma_alloc_pin(
                            dma_man,
                            sizeof(struct descriptor) * dev->tx_size,
                            0,
                            DMA_ALIGN);
    if (!tx_ring.phys) {
        LOG_ERROR("Failed to allocate tx_ring");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    ps_dma_cache_clean_invalidate(
        dma_man,
        tx_ring.virt,
        sizeof(struct descriptor) * dev->tx_size);

    dev->rx_cookies = malloc(sizeof(void *) * dev->rx_size);
    dev->tx_cookies = malloc(sizeof(void *) * dev->tx_size);
    dev->tx_lengths = malloc(sizeof(unsigned int) * dev->tx_size);
    if (!dev->rx_cookies || !dev->tx_cookies || !dev->tx_lengths) {
        if (dev->rx_cookies) {
            free(dev->rx_cookies);
        }
        if (dev->tx_cookies) {
            free(dev->tx_cookies);
        }
        if (dev->tx_lengths) {
            free(dev->tx_lengths);
        }
        LOG_ERROR("Failed to malloc");
        free_desc_ring(dev, dma_man);
        return -1;
    }
    dev->tx_ring = tx_ring.virt;
    dev->tx_ring_phys = tx_ring.phys;
    /* Remaining needs to be 2 less than size as we cannot actually enqueue
     * size many descriptors, since then the head and tail pointers would be
     * equal, indicating empty.
     */
    dev->rx_remain = dev->rx_size - 2;
    dev->tx_remain = dev->tx_size - 2;

    dev->rdt = dev->rdh = dev->tdt = dev->tdh = 0;

    /* zero both rings */
    for (unsigned int i = 0; i < dev->tx_size; i++) {
        dev->tx_ring[i] = (struct descriptor) {
            .phys = 0,
            .len = 0,
            .stat = (i + 1 == dev->tx_size) ? TXD_WRAP : 0
        };
    }
    for (unsigned int i = 0; i < dev->rx_size; i++) {
        dev->rx_ring[i] = (struct descriptor) {
            .phys = 0,
            .len = 0,
            .stat = (i + 1 == dev->rx_size) ? RXD_WRAP : 0
        };
    }
    __sync_synchronize();

    return 0;
}

/*----------------------------------------------------------------------------*/
static void
complete_rx(
    struct eth_driver *driver)
{
    struct imx6_eth_data *dev = get_dev_from_driver(driver);
    unsigned int rdt = dev->rdt;
    while (dev->rdh != rdt) {
        unsigned int status = dev->rx_ring[dev->rdh].stat;
        /* Ensure no memory references get ordered before we checked the
         * descriptor was written back
         */
        __sync_synchronize();
        if (status & RXD_EMPTY) {
            /* not complete yet */
            break;
        }
        void *cookie = dev->rx_cookies[dev->rdh];
        unsigned int len = dev->rx_ring[dev->rdh].len;
        /* update rdh */
        dev->rdh = (dev->rdh + 1) % dev->rx_size;
        dev->rx_remain++;
        /* Give the buffers back */
        driver->i_cb.rx_complete(driver->cb_cookie, 1, &cookie, &len);
    }
    struct enet *enet = dev->enet;
    if (dev->rdt != dev->rdh && !enet_rx_enabled(enet)) {
        enet_rx_enable(enet);
    }
}

/*----------------------------------------------------------------------------*/
static void
complete_tx(
    struct eth_driver *driver)
{
    struct imx6_eth_data *dev = get_dev_from_driver(driver);
    while (dev->tdh != dev->tdt) {
        unsigned int i;
        for (i = 0; i < dev->tx_lengths[dev->tdh]; i++) {
            if (dev->tx_ring[(i + dev->tdh) % dev->tx_size].stat & TXD_READY) {
                /* not all parts complete */
                return;
            }
        }
        /* do not let memory loads happen before our checking of the descriptor
         * write back
         */
        __sync_synchronize();
        /* increase where we believe tdh to be */
        void *cookie = dev->tx_cookies[dev->tdh];
        dev->tx_remain += dev->tx_lengths[dev->tdh];
        dev->tdh = (dev->tdh + dev->tx_lengths[dev->tdh]) % dev->tx_size;
        /* give the buffer back */
        driver->i_cb.tx_complete(driver->cb_cookie, cookie);
    }
    if (dev->tdh != dev->tdt && !enet_tx_enabled(dev->enet)) {
        enet_tx_enable(dev->enet);
    }
}

/*----------------------------------------------------------------------------*/
static void
print_state(
    struct eth_driver *driver)
{
    struct enet *enet = get_enet_from_driver(driver);
    enet_print_mib(enet);
}

/*----------------------------------------------------------------------------*/
static void handle_irq(
    struct eth_driver *driver,
    int irq)
{
    struct enet *enet = get_enet_from_driver(driver);
    uint32_t e;
    e = enet_clr_events(enet, IRQ_MASK);
    if (e & NETIRQ_TXF) {
        complete_tx(driver);
    }
    if (e & NETIRQ_RXF) {
        complete_rx(driver);
        fill_rx_bufs(driver);
    }
    if (e & NETIRQ_EBERR) {
        LOG_ERROR("Error: System bus/uDMA");
        //ethif_print_state(netif_get_eth_driver(netif));
        assert(0);
        while (1);
    }
}

/*----------------------------------------------------------------------------*/
/* This is a platsuport IRQ interface IRQ handler wrapper for handle_irq() */
static void
eth_irq_handle(
    void *data,
    ps_irq_acknowledge_fn_t acknowledge_fn,
    void *ack_data)
{
    if (data == NULL)
    {
        LOG_ERROR("IRQ handler got data=NULL");
        assert(0);
        return;
    }

    struct eth_driver *driver = data;

    /* handle_irq doesn't really expect an IRQ number */
    handle_irq(driver, 0);

    int error = acknowledge_fn(ack_data);
    if (error) {
        LOG_ERROR("Failed to acknowledge the Ethernet device's IRQ, code %d", error);
    }
}


/*----------------------------------------------------------------------------*/
static void
raw_poll(
    struct eth_driver *driver)
{
    complete_rx(driver);
    complete_tx(driver);
    fill_rx_bufs(driver);
}

/*----------------------------------------------------------------------------*/
static int
raw_tx(
    struct eth_driver *driver,
    unsigned int num,
    uintptr_t *phys,
    unsigned int *len,
    void *cookie)
{
    struct enet *enet = get_enet_from_driver(driver);
    struct imx6_eth_data *dev = get_dev_from_driver(driver);

    /* Ensure we have room */
    if (dev->tx_remain < num) {
        /* try and complete some */
        complete_tx(driver);
        if (dev->tx_remain < num) {
            LOG_ERROR("TX queue lacks space, have %d, need %d", dev->tx_remain, num);
            return ETHIF_TX_FAILED;
        }
    }
    unsigned int i;
    __sync_synchronize();
    for (i = 0; i < num; i++) {
        unsigned int ring = (dev->tdt + i) % dev->tx_size;
        dev->tx_ring[ring].len = len[i];
        dev->tx_ring[ring].phys = phys[i];
        __sync_synchronize();
        dev->tx_ring[ring].stat = TXD_READY
                                  | (ring + 1 == dev->tx_size ? TXD_WRAP : 0)
                                  | (i + 1 == num ? TXD_ADDCRC | TXD_LAST : 0);
    }
    dev->tx_cookies[dev->tdt] = cookie;
    dev->tx_lengths[dev->tdt] = num;
    dev->tdt = (dev->tdt + num) % dev->tx_size;
    dev->tx_remain -= num;
    __sync_synchronize();
    if (!enet_tx_enabled(enet)) {
        enet_tx_enable(enet);
    }

    return ETHIF_TX_ENQUEUED;
}

/*----------------------------------------------------------------------------*/
static void
get_mac(
    struct eth_driver *driver,
    uint8_t *mac)
{
    struct enet *enet = get_enet_from_driver(driver);
    enet_get_mac(enet, (unsigned char *)mac);
}

/*----------------------------------------------------------------------------*/
static struct raw_iface_funcs iface_fns = {
    .raw_handleIRQ = handle_irq,
    .print_state = print_state,
    .low_level_init = low_level_init,
    .raw_tx = raw_tx,
    .raw_poll = raw_poll,
    .get_mac = get_mac
};

/*----------------------------------------------------------------------------*/
static int
obtain_mac(
    uint8_t* mac,
    const nic_config_t* nic_config,
    ps_io_ops_t *io_ops)
{
    if (nic_config && (0 != memcmp(
                                nic_config->mac,
                                "\x00\x00\x00\x00\x00\x00",
                                sizeof(nic_config->mac)))) {
        LOG_INFO("obtain MAC from nic_config");
        memcpy(mac, nic_config->mac, sizeof(nic_config->mac));
        return 0;
    }

    /* read MAC from eFuses */
    LOG_INFO("obtain MAC from OCOTP");
    struct ocotp *ocotp = ocotp_init(&io_ops->io_mapper);
    if (!ocotp) {
        LOG_ERROR("Failed to initialize OCOTP");
        return -1;
    }
    unsigned int enet_id = nic_config ? nic_config->id : 0;
    int err = ocotp_get_mac(ocotp, enet_id, mac);
    ocotp_free(ocotp, &io_ops->io_mapper);
    if (err) {
        LOG_ERROR("Failed to get MAC for ID %d from OCOTP, code %d", enet_id, err);
        return -1;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
static int
ethif_imx6_init(
    struct eth_driver *driver,
    ps_io_ops_t *io_ops,
    const nic_config_t* nic_config)
{
    int err;

    /* configuration */
    unsigned int enet_id = nic_config ? nic_config->id : 0;

    uint8_t mac[6];
    err = obtain_mac(mac, nic_config, io_ops);
    if (err) {
        LOG_ERROR("Failed to obtain MAC, code %d", err);
        return -1;
    }
    LOG_INFO("using MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    struct imx6_eth_data *dev = (struct imx6_eth_data *)malloc(
                                    sizeof(struct imx6_eth_data));
    if (!dev) {
        LOG_ERROR("Failed to allocate eth data struct");
        return -1;
    }
    /* dev got allocated, we need to free it on error */

    dev->tx_size = CONFIG_LIB_ETHDRIVER_RX_DESC_COUNT;
    dev->rx_size = CONFIG_LIB_ETHDRIVER_TX_DESC_COUNT;
    driver->eth_data = dev;
    driver->dma_alignment = DMA_ALIGN;
    driver->i_fn = iface_fns;

    err = initialize_desc_ring(dev, &io_ops->dma_manager);
    if (err) {
        LOG_ERROR("Failed to allocate descriptor rings, code %d", err);
        goto error;
    }

    /* Initialise ethernet pins, also does a PHY reset */
    if (0 != enet_id) {
        LOG_ERROR("Unsupported ENET ID %u", enet_id);
        goto error;
    }
    err = setup_iomux_enet(io_ops);
    if (err) {
        LOG_ERROR("Failed to setup IOMUX for ENET id=%d, code %d", enet_id, err);
        goto error;
    }

    /* Map in the device */
    void *enet_mapping = NULL;

    switch (enet_id)
    {
        case 0:
            enet_mapping = RESOURCE(&io_ops->io_mapper, IMX6_ENET);
            break;

        default:
            LOG_ERROR("Unsupported ENET ID %u", enet_id);
            goto error;
    }

    if (!enet_mapping) {
        LOG_ERROR("ethernet controller ENET %u could not be mapped", enet_id);
        goto error;
    }

    /* Initialise the RGMII interface */
    struct enet *enet = enet_init(
                            enet_mapping,
                            dev->tx_ring_phys,
                            dev->rx_ring_phys,
                            BUF_SIZE,
                            mac,
                            io_ops);
    if (!enet) {
        LOG_ERROR("Failed to initialize RGMII");
        /* currently no way to properly clean up enet */
        assert(!"enet cannot be cleaned up");
        goto error;
    }
    dev->enet = enet;

    /* enable promiscuous mode by default if nothing else is specified */
    if (!nic_config || nic_config->promiscuous_mode) {
        enet_prom_enable(enet);
    }
    else {
        enet_prom_disable(enet);
    }

    /* Initialise the phy library */
    miiphy_init();
    /* Initialise the phy */
    phy_micrel_init();
    /* Connect the phy to the ethernet controller */
    unsigned int phy_mask = 0xffffffff;
    if (nic_config && (0 != nic_config->phy_address))
    {
        LOG_INFO("using PHY address %d from config", nic_config->phy_address);
        phy_mask = BIT(nic_config->phy_address);
    }
    struct phy_device *phydev = fec_init(phy_mask, enet);
    if (!phydev) {
        LOG_ERROR("Failed to initialize fec");
        goto error;
    }

    enet_set_speed(
        enet,
        phydev->speed,
        (phydev->duplex == DUPLEX_FULL) ? 1 : 0);

    printf("\n  * Link speed: %d Mbps, %s-duplex *\n\n",
           phydev->speed,
           (phydev->duplex == DUPLEX_FULL) ? "full" : "half");

    udelay(100000); // why?

    /* Start the controller */
    enet_enable(enet);

    fill_rx_bufs(driver);

    /* enable interrupts */
    enet_enable_events(enet, 0);
    enet_clr_events(enet, (uint32_t)~IRQ_MASK);
    enet_enable_events(enet, (uint32_t)IRQ_MASK);

    /* done */
    return 0;

error:
    // ToDo: free enet
    assert(dev);
    free_desc_ring(dev, &io_ops->dma_manager);
    free(dev);
    return -1;
}

/*----------------------------------------------------------------------------*/
typedef struct {
    ps_io_ops_t *io_ops;
    struct eth_driver *eth_driver;
    void *addr;
    int irq_id;
} callback_args_t;

/*----------------------------------------------------------------------------*/
static int
allocate_register_callback(
    pmem_region_t pmem,
    unsigned curr_num,
    size_t num_regs,
    void *token)
{
    if (token == NULL) {
        LOG_ERROR("Expected a token!");
        return -EINVAL;
    }

    /* we support only one peripheral, ignore others gracefully */
    if (curr_num != 0) {
        ZF_LOGW("Ignoring peripheral #%d at 0x%"PRIx64, curr_num, pmem.base_addr);
        return 0;
    }

    callback_args_t *args = token;
    void *addr = ps_pmem_map(args->io_ops, pmem, false, PS_MEM_NORMAL);
    if (!addr) {
        LOG_ERROR("Failed to map the Ethernet device");
        return -EIO;
    }

    args->addr = addr;
    return 0;
}

/*----------------------------------------------------------------------------*/
static int
allocate_irq_callback(
    ps_irq_t irq,
    unsigned curr_num,
    size_t num_irqs,
    void *token)
{
    if (token == NULL) {
        LOG_ERROR("Expected a token!");
        return -EINVAL;
    }

    /* we support only one interrupt, ignore others gracefully */
    if (curr_num != 0) {
        ZF_LOGW("Ignoring interrupt #%d with value %d", curr_num, irq);
        return 0;
    }

    callback_args_t *args = token;
    irq_id_t irq_id = ps_irq_register(
                        &args->io_ops->irq_ops,
                        irq,
                        eth_irq_handle,
                        args->eth_driver);
    if (irq_id < 0) {
        LOG_ERROR("Failed to register the Ethernet device's IRQ");
        return -EIO;
    }

    args->irq_id = irq_id;
    return 0;
}

/*----------------------------------------------------------------------------*/
int
ethif_imx_init_module(
    ps_io_ops_t *io_ops,
    const char *device_path)
{
    /* get a configuration if function is implemented */
    const nic_config_t* nic_config = NULL;
    if (get_nic_configuration) {
        LOG_INFO("calling get_nic_configuration()");
        nic_config = get_nic_configuration();
    }

    struct eth_driver *eth_driver = NULL;
    int error = ps_calloc(
                    &io_ops->malloc_ops,
                    1,
                    sizeof(*eth_driver),
                    (void **) &eth_driver);
    if (error) {
        LOG_ERROR("Failed to allocate memory for the Ethernet driver, code %d", error);
        return -ENOMEM;
    }

    ps_fdt_cookie_t *cookie = NULL;
    callback_args_t args = { .io_ops = io_ops, .eth_driver = eth_driver };
    error = ps_fdt_read_path(
                &io_ops->io_fdt,
                &io_ops->malloc_ops,
                device_path,
                &cookie);
    if (error) {
        LOG_ERROR("Failed to read the path of the Ethernet device, code %d", error);
        return -ENODEV;
    }

    error = ps_fdt_walk_registers(
                &io_ops->io_fdt,
                cookie,
                allocate_register_callback,
                &args);
    if (error) {
        LOG_ERROR("Failed to walk the Ethernet device's registers and allocate them, code %d", error);
        return -ENODEV;
    }

    error = ps_fdt_walk_irqs(
                &io_ops->io_fdt,
                cookie,
                allocate_irq_callback,
                &args);
    if (error) {
        LOG_ERROR("Failed to walk the Ethernet device's IRQs and allocate them, code %d", error);
        return -ENODEV;
    }

    error = ps_fdt_cleanup_cookie(&io_ops->malloc_ops, cookie);
    if (error) {
        LOG_ERROR("Failed to free the cookie used to allocate resources, code %d", error);
        return -ENODEV;
    }

    error = ethif_imx6_init(eth_driver, io_ops, nic_config);
    if (error) {
        LOG_ERROR("Failed to initialise the Ethernet driver, code %d", error);
        return -ENODEV;
    }

    error = ps_interface_register(
                &io_ops->interface_registration_ops,
                PS_ETHERNET_INTERFACE,
                eth_driver,
                NULL);
    if (error) {
        LOG_ERROR("Failed to register Ethernet driver interface , code %d", error);
        return -ENODEV;
    }

    return 0;
}

/*----------------------------------------------------------------------------*/
static const char *compatible_strings[] = {
    /* Other i.MX platforms may also be compatible but the platforms that have
     * been tested are the SABRE Lite (i.MX6Quad) and i.MX8MQ Evaluation Kit
     */
    "fsl,imx6q-fec",
    "fsl,imx8mq-fec",
    NULL
};

PS_DRIVER_MODULE_DEFINE(imx_fec, compatible_strings, ethif_imx_init_module);
