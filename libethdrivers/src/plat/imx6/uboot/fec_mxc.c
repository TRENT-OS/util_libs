/*
 * @TAG(OTHER_GPL)
 */

/*
 * (C) Copyright 2009 Ilya Yanok, Emcraft Systems Ltd <yanok@emcraft.com>
 * (C) Copyright 2008,2009 Eric Jarrige <eric.jarrige@armadeus.org>
 * (C) Copyright 2008 Armadeus Systems nc
 * (C) Copyright 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2007 Pengutronix, Juergen Beisert <j.beisert@pengutronix.de>
 * (C) Copyright 2018, NXP
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include "common.h"
#include "miiphy.h"
#include "fec_mxc.h"

#include "imx-regs.h"
#include "../io.h"

#include "micrel.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "../enet.h"
#include "../ocotp_ctrl.h"
#include <utils/attribute.h>

#undef DEBUG

static int fec_phy_read(
    struct mii_dev *bus,
    int phyAddr,
    UNUSED int dev_addr,
    int regAddr)
{
    return enet_mdio_read((struct enet *)bus->priv, phyAddr, regAddr);
}

static int fec_phy_write(
    struct mii_dev *bus,
    int phyAddr,
    UNUSED int dev_addr,
    int regAddr,
    uint16_t data)
{
    return enet_mdio_write((struct enet *)bus->priv, phyAddr, regAddr, data);
}

int cb_phy_read(
    struct mii_dev *bus,
    UNUSED int phyAddr,
    UNUSED int dev_addr,
    int regAddr)
{
    nic_config_t* nic_config = (nic_config_t*)bus->priv;
    if ((!nic_config) || (!nic_config->funcs.mdio_read))
    {
        LOG_ERROR("mdio_read() from nic_config not set");
        assert(nic_config); // we should never be here if nic_config is NULL
        return -1;
    }
    return nic_config->funcs.mdio_read(regAddr);
}

int cb_phy_write(
    struct mii_dev *bus,
    UNUSED int phyAddr,
    UNUSED int dev_addr,
    int regAddr,
    uint16_t data)
{
    nic_config_t* nic_config = (nic_config_t*)bus->priv;
    if ((!nic_config) || (!nic_config->funcs.mdio_write))
    {
        LOG_ERROR("mdio_write() from nic_config not set");
        assert(nic_config); // we should never be here if nic_config is NULL
        return -1;
    }

    return nic_config->funcs.mdio_write(regAddr, data);
}

// /**
//  * Halt the FEC engine
//  * @param[in] dev Our device to handle
//  */
// void fec_halt(struct eth_device *dev)
// {
// #if 0
//     struct fec_priv *fec = (struct fec_priv *)dev->priv;
//     int counter = 0xffff;
//     /* issue graceful stop command to the FEC transmitter if necessary */
//     writel(FEC_TCNTRL_GTS | readl(&fec->eth->x_cntrl), &fec->eth->x_cntrl);
//     /* wait for graceful stop to register */
//     while ((counter--) && (!(readl(&fec->eth->ievent) & FEC_IEVENT_GRA))) {
//         udelay(1);
//     }
//     writel(readl(&fec->eth->ecntrl) & ~FEC_ECNTRL_ETHER_EN, &fec->eth->ecntrl);
//     fec->rbd_index = 0;
//     fec->tbd_index = 0;
// #else
//     assert(!"unimplemented");
// #endif
// }

struct phy_device * fec_init(
    unsigned int phy_mask,
    struct enet* enet,
    const nic_config_t* nic_config)
{
    int ret;

    /* Allocate the mdio bus */
    struct mii_dev *bus = mdio_alloc();
    if (!bus) {
        LOG_ERROR("Could not allocate MDIO");
        return NULL;
    }

#ifdef CONFIG_PLAT_IMX6SX
    // on the i.MX6 SoloX Nitrogen board, both PHYs are connected to enet1's
    // MDIO, while enet2's MDIO pins are used for other I/O purposes.
    strncpy(bus->name, "MDIO-ENET1", sizeof(bus->name));
#else
    strncpy(bus->name, "MDIO", sizeof(bus->name));
#endif

    /* if we don't have direct access to MDIO, use the callbacks from config */
    if (!enet && !nic_config)
    {
        LOG_ERROR("Neither ENET nor nic_config is set, can't access MDIO");
    }
    bus->priv = enet ? enet : (struct enet *)nic_config;
    bus->read = enet ? fec_phy_read : cb_phy_read;
    bus->write = enet ? fec_phy_write : cb_phy_write;

    ret = mdio_register(bus);
    if (ret) {
        LOG_ERROR("Could not register MDIO, code %d", ret);
        free(bus);
        return NULL;
    }

    /* ***** Configure phy *****/
    struct eth_device edev = { .name = "DUMMY-EDEV" }; // just a dummy
    struct phy_device *phydev= phy_connect_by_mask(
                                bus,
                                phy_mask,
                                &edev,
                                PHY_INTERFACE_MODE_RGMII);
    if (!phydev) {
        LOG_ERROR("Could not connect to PHY");
        return NULL;
    }

#if defined(CONFIG_PLAT_IMX8MQ_EVK)

    /* enable rgmii rxc skew and phy mode select to RGMII copper */
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x1f);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x8);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x100);

    if (phydev->drv->config) {
        phydev->drv->config(phydev);
    }

    if (phydev->drv->startup) {
        phydev->drv->startup(phydev);
    }

#elif defined(CONFIG_PLAT_IMX6SX)

    LOG_INFO("IMX6SX: Initializing PHY for Atheros");
    int val;

    /*
     * Ar803x phy SmartEEE feature cause link status generates glitch,
     * which cause ethernet link down/up issue, so disable SmartEEE
     */
    phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x3);
    phy_write(phydev, MDIO_DEVAD_NONE, 0xe, 0x805d);
    phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x4003);
    val = phy_read(phydev, MDIO_DEVAD_NONE, 0xe);
    phy_write(phydev, MDIO_DEVAD_NONE, 0xe, val & ~(1 << 8));

    // /* To enable AR8031 ouput a 125MHz clk from CLK_25M */
    // phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x7);
    // phy_write(phydev, MDIO_DEVAD_NONE, 0xe, 0x8016);
    // phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x4007);

    // val = phy_read(phydev, MDIO_DEVAD_NONE, 0xe);
    // val &= 0xffe3;
    // val |= 0x18;
    // phy_write(phydev, MDIO_DEVAD_NONE, 0xe, val);

    /* rgmii tx clock delay enable */
    // phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
    // val = phy_read(phydev, MDIO_DEVAD_NONE, 0x1e);
    // phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, (val|0x0100));

    // phydev->supported = phydev->drv->features;

    if (phydev->drv->config) {
        phydev->drv->config(phydev);
    }

    if (phydev->drv->startup) {
        phydev->drv->startup(phydev);
    }

#elif defined(CONFIG_PLAT_IMX6)

    /* min rx data delay */
    ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_RX_DATA_SKEW, 0x0);
    /* min tx data delay */
    ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_TX_DATA_SKEW, 0x0);
    /* max rx/tx clock delay, min rx/tx control */
    ksz9021_phy_extended_write(phydev, MII_KSZ9021_EXT_RGMII_CLOCK_SKEW, 0xf0f0);
    ksz9021_config(phydev);

    /* Start up the PHY */
    ret = ksz9021_startup(phydev);
    if (ret) {
        LOG_ERROR("Could not initialize PHY '%s', code %d", phydev->dev->name, ret);
        return NULL;
    }

#else
#error "unsupported platform"
#endif

    return phydev;
}
