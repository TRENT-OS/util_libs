/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

enum mux_feature {
    MUX_I2C1,
    MUX_I2C2,
    MUX_I2C3,
#if defined(CONFIG_PLAT_IMX6DQ)
    MUX_GPIO0_CLKO1,
#elif defined(CONFIG_PLAT_IMX6SX)
    MUX_GPIO11_CLKO1,
#else
#error "unknown i.MX6 SOC"
#endif
    MUX_UART1,
    NMUX_FEATURES
};

/**
 * Initialise the mux subsystem with pre-mapped regions.
 * @param[in]  iomuxc A virtual mapping for IOMUXC of the MUX subsystem.
 * @param[out] mux    On success, this will be filled with the appropriate
 *                    subsystem data.
 * @return            0 on success
 */
int imx6_mux_init(void* iomuxc, mux_sys_t* mux);

#ifdef CONFIG_PLAT_IMX6SX
/**
 * Initialise the mux subsystem with pre-mapped regions, but consider split regions of IOMUXC and IOMUXC_GPR.
 * @param[in]  iomuxc A virtual mapping for IOMUXC of the MUX subsystem.
 * @param[in]  iomuxc_gpr A virtual mapping for IOMUXC_GPR of the MUX subsystem.
 * @param[out] mux    On success, this will be filled with the appropriate
 *                    subsystem data.
 * @return            0 on success
 */
int imx6sx_mux_init_split(void* iomuxc, void* iomuxc_gpr, mux_sys_t* mux);
#endif
