/*
 * Copyright (C) 2022, HENSOLDT Cyber GmbH
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platsupport/reset.h>
#include <platsupport/plat/reset.h>

#define TX1_CLKCAR_PADDR 0x60006000
#define TX1_CLKCAR_SIZE 0x1000

/* This maps respective clock IDs to corresponding clock control register according to Tegra X1 TRM */

uint32_t reset_id_ctrl_register_map[] = {
    [SWR_UARTA_RST] = CLK_RST_CONTROLLER_RST_DEVICES_L_0,
    [SWR_UARTB_RST] = CLK_RST_CONTROLLER_RST_DEVICES_L_0,
    [SWR_UARTC_RST] = CLK_RST_CONTROLLER_RST_DEVICES_H_0,
    [SWR_UARTD_RST] = CLK_RST_CONTROLLER_RST_DEVICES_U_0,
};

typedef struct tx1_reset {
    ps_io_ops_t *io_ops;
    void *car_vaddr;
} tx1_reset_t;

static inline bool check_valid_reset(reset_id_t id) {
    return (SWR_UARTA_RST <= id && id < NRESETS);
}

static int tx1_reset_common(void *data, reset_id_t id, bool assert) {
    if(!check_valid_reset(id)) {
        ZF_LOGE("Invalid reset ID");
        return -EINVAL;
    }

    tx1_reset_t *tx1_reset = data;

    // Get corresponding reset control register (one of RST_DEVICES_L/H/U/V/W/X/Y) for given reset ID
    uint32_t offset = reset_id_ctrl_register_map[id];

    // Get respective bit to set in the register; this means calculating the reset ID modulo 32
    uint32_t bit = id % 32;

    // Assert or deassert the reset by setting the identified bit within respective RST_DEVICES_L/H/U/V/W/X/Y register
    if(assert) {
        *((volatile uint32_t*)((uintptr_t)tx1_reset->car_vaddr + offset)) |= BIT(bit);
    }
    else {
        *((volatile uint32_t*)((uintptr_t)tx1_reset->car_vaddr + offset)) &= ~BIT(bit);
    }

    return 0;
}

static int tx1_reset_assert(void *data, reset_id_t id) {
    return tx1_reset_common(data, id, true);
}

static int tx1_reset_deassert(void *data, reset_id_t id) {
    return tx1_reset_common(data, id, false);
}

int reset_sys_init(ps_io_ops_t *io_ops, void *dependencies, reset_sys_t *reset_sys) {
    if(!io_ops || !reset_sys) {
        if(!io_ops) {
            ZF_LOGE("null io_ops_argument");
        }

        if(!reset_sys) {
            ZF_LOGE("null reset_sys argument");
        }

        return -EINVAL;
    }

    int error = 0;
    tx1_reset_t *reset = NULL;
    void *car_vaddr = NULL;

    error = ps_calloc(&io_ops->malloc_ops, 1, sizeof(tx1_reset_t), (void **) &reset_sys->data);
    if(error) {
        ZF_LOGE("Failed to allocate memory for reset sys internal structure");
        error = -ENOMEM;
        goto fail;
    }

    reset = reset_sys->data;

    // Check if the underlying TX1 CAR device was already mapped by the clock subsystem before.
    // Otherwise, initialise here.
    // if(!(&io_ops->clock_sys)) {
        car_vaddr = ps_io_map(&io_ops->io_mapper, TX1_CLKCAR_PADDR, TX1_CLKCAR_SIZE, 0, PS_MEM_NORMAL);
        if(car_vaddr == NULL) {
            ZF_LOGE("Failed to map TX1 CAR registers");
            error = -ENOMEM;
            goto fail;
        }

        reset->car_vaddr = car_vaddr;
    // }
    // else {
    //     ZF_LOGE("reset_sys_init() - clock_sys already initialized, car_vaddr set");
    //     tx1_clk_t *tx1_clk = io_ops->clock_sys.priv;
    //     reset->car_vaddr = tx1_clk->car_vaddr;
    // }

    reset->io_ops = io_ops;


    reset_sys->reset_assert = &tx1_reset_assert;
    reset_sys->reset_deassert = &tx1_reset_deassert;

    return 0;

fail:

    if(reset_sys->data) {
        ZF_LOGF_IF(ps_free(&io_ops->malloc_ops, sizeof(tx1_reset_t), (void *) reset_sys->data),
                    "Failed to free the reset private data after a failed reset subsystem initialisation");
    }

    return error;
}