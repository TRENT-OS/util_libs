/*
 * Atheros PHY drivers
 *
 * SPDX-License-Identifier:    GPL-2.0+
 *
 * Copyright 2011, 2013 Freescale Semiconductor, Inc.
 * author Andy Fleming
 */
#include "config.h"
#include "phy.h"
#include "common.h"

static int ar8021_config(struct phy_device *phydev)
{
    // printf("Called ar8021_config()\n");
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, 0x3D47);

    phydev->supported = phydev->drv->features;
    return 0;
}

static int ar8035_config(struct phy_device *phydev)
{
    // printf("Called ar8035_config() in atheros.c\n");
    int regval;

    phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x0007);
    phy_write(phydev, MDIO_DEVAD_NONE, 0xe, 0x8016);
    phy_write(phydev, MDIO_DEVAD_NONE, 0xd, 0x4007);

    regval = phy_read(phydev, MDIO_DEVAD_NONE, 0xe);
    // regval &= ~0x11c;
    // regval |= 0x80;    /* 1/2 drive strength */
    // regval |= 0x18;
    // printf("regval in atheros.c is %u.\n", regval);
    // phy_write(phydev, MDIO_DEVAD_NONE, 0xe, regval);
    phy_write(phydev, MDIO_DEVAD_NONE, 0xe, (regval|0x0018));

    regval = phy_read(phydev, MDIO_DEVAD_NONE, 0xe);
    // printf("regval in atheros.c is %u.\n", regval);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1d, 0x05);

    regval = phy_read(phydev, MDIO_DEVAD_NONE, 0x1e);
    phy_write(phydev, MDIO_DEVAD_NONE, 0x1e, (regval|0x0100));

    if ((phydev->interface == PHY_INTERFACE_MODE_RGMII_ID) ||
        (phydev->interface == PHY_INTERFACE_MODE_RGMII_TXID)) {
        /* select debug reg 5 */
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1D, 0x5);
        /* enable tx delay */
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1E, 0x0100);
    }

    if ((phydev->interface == PHY_INTERFACE_MODE_RGMII_ID) ||
        (phydev->interface == PHY_INTERFACE_MODE_RGMII_RXID)) {
        /* select debug reg 0 */
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1D, 0x0);
        /* enable rx delay */
        phy_write(phydev, MDIO_DEVAD_NONE, 0x1E, 0x8000);
    }

    unsigned ctrl1000 = 0;
    unsigned features = phydev->drv->features;

    // if (getenv("disable_giga"))
    // {
    //     features &= ~(SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full);
    // }
    if (features & SUPPORTED_1000baseT_Half)
    {
        ctrl1000 |= ADVERTISE_1000HALF;
    }
    if (features & SUPPORTED_1000baseT_Full)
    {
        ctrl1000 |= ADVERTISE_1000FULL;
    }
    phydev->advertising = phydev->supported = features;
    phy_write(phydev, MDIO_DEVAD_NONE, MII_CTRL1000, ctrl1000);
    genphy_config_aneg(phydev);
    genphy_restart_aneg(phydev);

    return 0;
}

static struct phy_driver AR8021_driver =  {
    .name = "AR8021",
    .uid = 0x4dd040,
    .mask = 0x4ffff0,
    .features = PHY_GBIT_FEATURES,
    .config = ar8021_config,
    .startup = genphy_startup,
    .shutdown = genphy_shutdown,
};

static struct phy_driver AR8031_driver =  {
    .name = "AR8031/AR8033",
    .uid = 0x4dd074,
    .mask = 0xffffffef,
    .features = PHY_GBIT_FEATURES,
    .config = ar8035_config,
    .startup = genphy_startup,
    .shutdown = genphy_shutdown,
};

static struct phy_driver AR8035_driver =  {
    .name = "AR8035",
    .uid = 0x4dd072,
    .mask = 0xffffffef,
    .features = PHY_GBIT_FEATURES,
    .config = ar8035_config,
    .startup = genphy_startup,
    .shutdown = genphy_shutdown,
};

int phy_atheros_init(void)
{
    // printf("Called phy_atheros_init()\n");
    phy_register(&AR8021_driver);
    phy_register(&AR8031_driver);
    phy_register(&AR8035_driver);

    return 0;
}
