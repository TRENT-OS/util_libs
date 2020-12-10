/*
 * Copyright 2017, DORNERWORKS
 * Copyright 2020, Hensoldt Cyber
 *
 * SPDX-License-Identifier: GPL2.0+
 */

#pragma once

#include <utils/attribute.h>
#include <ethdrivers/raw.h>
#include "../src/plat/imx6/enet.h"


struct enet * get_enet_from_driver(struct eth_driver *driver);

typedef int (*sync_func_t)(void);
typedef int (*mdio_read_func_t)(uint16_t reg);
typedef int (*mdio_write_func_t)(uint16_t reg, uint16_t data);

typedef struct
{
    unsigned int phy_address; /**< Address of the PHY chip */
    unsigned int promiscuous_mode; /**< Active promiscuous mode on the NIC */
    unsigned int id; /**< ID of the NIC instance */
    unsigned char mac[6]; /**< MAC address to set. Pass all 0 to read it from
    the hardware registers.  */
    struct {
        sync_func_t sync; /**< blocks until primary NIC init is finished */
        mdio_read_func_t mdio_read; /**< primary NIC mdio_read rpc function */
        mdio_write_func_t mdio_write; /**< primary NIC mdio_write rpc function */
    } funcs;
} nic_config_t;

WEAK const nic_config_t* get_nic_configuration(void);

