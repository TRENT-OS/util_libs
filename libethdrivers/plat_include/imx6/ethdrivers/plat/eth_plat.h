/*
 * Copyright 2017, DORNERWORKS
 * Copyright 2020, Hensoldt Cyber
 *
 * SPDX-License-Identifier: GPL2.0+
 */

#pragma once

#include <utils/attribute.h>


typedef struct
{
    unsigned int phy_address; /**< Address of the PHY chip */
    unsigned int promiscuous_mode; /**< Active promiscuous mode on the NIC */
    unsigned int id; /**< ID of the NIC instance */
    unsigned char mac[6]; /**< MAC address to set. Pass all 0 to read it from
    the hardware registers.  */
} nic_config_t;

WEAK const nic_config_t* get_nic_configuration(void);

