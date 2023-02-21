/*
 * Copyright (C) 2022, HENSOLDT Cyber GmbH
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdint.h>

/* The Nvidia Tegra X1 TRM describes the individual CAR registers in chapter 5.2. */

// Contains the absolute values for bit fields within the registers defined in reset_ctrl_register.
// Currently only filled for UART, further modules have to be added based on TRM chapter 5.2 if required.
enum reset_id {
    SWR_UARTA_RST = 6,  // CLK_RST_CONTROLLER_RST_DEVICES_L_0
    SWR_UARTB_RST = 7,  // CLK_RST_CONTROLLER_RST_DEVICES_L_0
    SWR_UARTC_RST = 55, // CLK_RST_CONTROLLER_RST_DEVICES_H_0 -> 32 + 23
    SWR_UARTD_RST = 65, // CLK_RST_CONTROLLER_RST_DEVICES_U_0 -> 32 + 32 +1
    NRESETS
};


// For the reset controller, only the RST_DEVICES_L/H/U/V/W/X/Y registers are relevant.
// They allow to assert/deassert a respective module.
// Corresponding offsets are listed below.
enum reset_ctrl_register {
    CLK_RST_CONTROLLER_RST_DEVICES_L_0 = 0x4,
    CLK_RST_CONTROLLER_RST_DEVICES_H_0 = 0x8,
    CLK_RST_CONTROLLER_RST_DEVICES_U_0 = 0xc,
    CLK_RST_CONTROLLER_RST_DEVICES_V_0 = 0x358,
    CLK_RST_CONTROLLER_RST_DEVICES_W_0 = 0x35c,
    CLK_RST_CONTROLLER_RST_DEVICES_X_0 = 0x28c,
    CLK_RST_CONTROLLER_RST_DEVICES_Y_0 = 0x2a4,
};

/* Alternate method for programming the RST_DEVICES_L/H/U/V/W/X/Y registers: use the
 * corresponding RST_DEV_L/H/U/V/W/X/Y_SET/CLR registers
 * - CLK_RST_CONTROLLER_RST_DEV_L_SET_0 -> offset: 0x300
 * - CLK_RST_CONTROLLER_RST_DEV_L_CLR_0 -> offset: 0x304
 * - CLK_RST_CONTROLLER_RST_DEV_H_SET_0 -> offset: 0x308
 * - CLK_RST_CONTROLLER_RST_DEV_H_CLR_0 -> offset: 0x30c
 * - CLK_RST_CONTROLLER_RST_DEV_U_SET_0 -> offset: 0x310
 * - CLK_RST_CONTROLLER_RST_DEV_U_CLR_0 -> offset: 0x314
 * - CLK_RST_CONTROLLER_RST_DEV_V_SET_0 -> offset: 0x430
 * - CLK_RST_CONTROLLER_RST_DEV_V_CLR_0 -> offset: 0x434
 * - CLK_RST_CONTROLLER_RST_DEV_W_SET_0 -> offset: 0x438
 * - CLK_RST_CONTROLLER_RST_DEV_W_CLR_0 -> offset: 0x43c
 * - CLK_RST_CONTROLLER_RST_DEV_X_SET_0 -> offset: 0x290
 * - CLK_RST_CONTROLLER_RST_DEV_X_CLR_0 -> offset: 0x294
 * - CLK_RST_CONTROLLER_RST_DEV_Y_SET_0 -> offset: 0x2a8
 * - CLK_RST_CONTROLLER_RST_DEV_Y_CLR_0 -> offset: 0x2ac
*/