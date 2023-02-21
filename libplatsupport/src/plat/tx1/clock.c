/*
 * Copyright (C) 2022, HENSOLDT Cyber GmbH
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platsupport/clock.h>
#include <platsupport/plat/clock.h>

/*
 * This is here just to make the compiler happy. The actual clock driver is
 * located inside libplatsupportports in the projects_libs repository.
 */
clk_t *ps_clocks = NULL;
freq_t ps_freq_default = {0};

#define TX1_CLKCAR_PADDR 0x60006000
#define TX1_CLKCAR_SIZE 0x1000

/* This maps respective clock IDs to corresponding clock control register according to Tegra X1 TRM */

uint32_t clk_id_ctrl_register_map[] = {
    [CLK_ENB_UARTA] = CLK_RST_CONTROLLER_CLK_OUT_ENB_L_0,
    [CLK_ENB_UARTB] = CLK_RST_CONTROLLER_CLK_OUT_ENB_L_0,
    [CLK_ENB_UARTC] = CLK_RST_CONTROLLER_CLK_OUT_ENB_H_0,
    [CLK_ENB_UARTD] = CLK_RST_CONTROLLER_CLK_OUT_ENB_U_0,
};

/* This maps respective clock IDs to corresponding clock source register according to Tegra X1 TRM */

uint32_t clk_id_src_register_map[] = {
    [CLK_ENB_UARTA] = CLK_RST_CONTROLLER_CLK_SOURCE_UARTA_0,
    [CLK_ENB_UARTB] = CLK_RST_CONTROLLER_CLK_SOURCE_UARTB_0,
    [CLK_ENB_UARTC] = CLK_RST_CONTROLLER_CLK_SOURCE_UARTC_0,
    [CLK_ENB_UARTD] = CLK_RST_CONTROLLER_CLK_SOURCE_UARTD_0,
};

typedef struct tx1_clk {
    ps_io_ops_t *io_ops;
    void *car_vaddr;
} tx1_clk_t;

static inline bool check_valid_gate(enum clock_gate gate) {
    return (CLK_GATE_ENB_ISPB <= gate && gate < NCLKGATES);
}

static inline bool check_valid_clk_id(enum clk_id id) {
    return (CLK_ENB_ISPB <= id && id < NCLOCKS);
}

// On TX1 there is no real clock gate but respective clock enable/disable registers (via CLK_OUT_ENB_L/H/U/V/W/X/Y) are provided
static int tx1_car_gate_enable(clock_sys_t *clock_sys, enum clock_gate gate, enum clock_gate_mode mode) {
    /* The TX1 CAR controller only supports enabling and disabling the clock
     * signal to a device: there are no idle/sleep clock modes, so we ignore
     * CLKGATE_IDLE and CLKGATE_SLEEP.
    */

    if(clock_sys == NULL) {
        ZF_LOGE("Invalid driver instance handle!");
        return -EINVAL;
    }

    if(!check_valid_gate(gate)) {
        ZF_LOGE("Invalid clock gate!");
        return -EINVAL;
    }

    if(mode == CLKGATE_IDLE || mode == CLKGATE_SLEEP) {
        ZF_LOGE("Idle and sleep gate modes are not supported");
        return -EINVAL;
    }

    // Get corresponding clock control register (one of CLK_OUT_ENB_L/H/U/V/W/X/Y) for given clock gate/clock ID
    uint32_t offset = clk_id_ctrl_register_map[gate];

    // Get respective bit to set in the register; this means calculating the gate/clock ID modulo 32
    uint32_t bit = gate % 32;

    // Enable the clock device by setting the identified bit within respective CLK_OUT_ENB_L/H/U/V/W/X/Y register
    tx1_clk_t *tx1_clk = clock_sys->priv;
    *((volatile uint32_t*)((uintptr_t)tx1_clk->car_vaddr + offset)) |= BIT(bit);

    return 0;
}

static freq_t tx1_car_get_freq(clk_t *clk) {
    // TODO: implement function
}

static freq_t tx1_car_set_freq(clk_t *clk, freq_t hz) {
    // The function currently only sets the respective CLK_SOURCE_<mod> register for a device to 0,
    // which means it provides an implicit set of the
    //  - clock divisor
    //  - clock source

    // Get corresponding clock source register (one of CLK_SOURCE_<mod>) for given clock gate/clock ID
    uint32_t offset = clk_id_src_register_map[clk->id];

    // Set clock divisor and clock source
    tx1_clk_t *tx1_clk = clk->clk_sys->priv;
    *((volatile uint32_t*)((uintptr_t)tx1_clk->car_vaddr + offset)) = 0;

    return 0;
}

static clk_t *tx1_car_get_clock(clock_sys_t *clock_sys, enum clk_id id) {
    if(!check_valid_clk_id(id)) {
        ZF_LOGE("Invalid clock ID");
        return NULL;
    }

    clk_t *ret_clk = NULL;
    tx1_clk_t *tx1_clk = clock_sys->priv;
    size_t clk_name_len = 0;
    int error = ps_calloc(&tx1_clk->io_ops->malloc_ops, 1, sizeof(*ret_clk), (void **) &ret_clk);
    if(error) {
        ZF_LOGE("Failed to allocate memory for the clock structure");
        return NULL;
    }

    bool clock_initialised = false;

    /* Enable the clock while we're at it */
    error = tx1_car_gate_enable(clock_sys, id, CLKGATE_ON);
    if(error) {
        goto fail;
    }

    clock_initialised = true;

    ret_clk->get_freq = tx1_car_get_freq;
    ret_clk->set_freq = tx1_car_set_freq;

    ret_clk->id = id;
    ret_clk->clk_sys = clock_sys;

    return ret_clk;

fail:
    if(ret_clk) {
        ps_free(&tx1_clk->io_ops->malloc_ops, sizeof(*ret_clk), (void *) ret_clk);
    }

    if(clock_initialised) {
        ZF_LOGF_IF(tx1_car_gate_enable(clock_sys, id, CLKGATE_OFF),
                    "Failed to disable clock following failed clock initialisation operation");
    }

    return NULL;
}

int clock_sys_init(ps_io_ops_t *io_ops, clock_sys_t *clock_sys) {
    if(!io_ops || !clock_sys) {
        if(!io_ops) {
            ZF_LOGE("null io_ops argument");
        }

        if(!clock_sys) {
            ZF_LOGE("null clock_sys argument");
        }

        return -EINVAL;
    }

    int error = 0;
    tx1_clk_t *clk = NULL;
    void *car_vaddr = NULL;

    error = ps_calloc(&io_ops->malloc_ops, 1, sizeof(tx1_clk_t), (void**) &clock_sys->priv);
    if(error) {
        ZF_LOGE("Failed to allocate memory for clock sys internal structure");
        error = -ENOMEM;
        goto fail;
    }

    clk = clock_sys->priv;

    car_vaddr = ps_io_map(&io_ops->io_mapper, TX1_CLKCAR_PADDR, TX1_CLKCAR_SIZE, 0, PS_MEM_NORMAL);
    if(car_vaddr == NULL) {
        ZF_LOGE("Failed to map TX1 CAR registers");
        error = -ENOMEM;
        goto fail;
    }

    clk->car_vaddr = car_vaddr;
    clk->io_ops = io_ops;

    clock_sys->gate_enable = &tx1_car_gate_enable;
    clock_sys->get_clock = &tx1_car_get_clock;

    return 0;

fail:

    if(car_vaddr) {
        ps_io_unmap(&io_ops->io_mapper, car_vaddr, TX1_CLKCAR_SIZE);
    }

    if(clock_sys->priv) {
        ZF_LOGF_IF(ps_free(&io_ops->malloc_ops, sizeof(tx1_clk_t), (void *)clock_sys->priv),
                    "Failed to free the clock private structure after failing to initialise");
    }

    return error;
}