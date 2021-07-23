/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2019-2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/timer.h>

#include <utils/util.h>
#include <stdint.h>
#include <stdbool.h>

/* The input frequence is the CPU frequency which is 240MHz by default */
#define APB_TIMER_INPUT_FREQ (240*1000*1000)
#define APB_TIMER0_PADDR   0x00409000
#define APB_TIMER1_PADDR   0x0040a000

/* Timer IRQs */
#define APB_TIMER_PLIC_BASE     15
#define APB_TIMER_IRQ_OVF(n)    APB_TIMER_PLIC_BASE + 2*n + 0x0
#define APB_TIMER_IRQ_CMP(n)    APB_TIMER_PLIC_BASE + 2*n + 0x1

// /* Multiple timers */
// #define APB_TIMER_DIST      0x10
// #define APB_TIMER_NUM       2
// #define APB_TIMER_BASE(n)   APB_TIMER_DIST * n

#define CMP_WIDTH               32
#define APB_TIMER_CTRL_ENABLE   BIT(0);
#define CMP_MASK                MASK(CMP_WIDTH)

typedef struct {
    /* vaddr apb_timer is mapped to */
    void *vaddr;
} apb_timer_config_t;

/* Memory map for apb_timer */
struct apb_timer_map {
    uint32_t time;
    uint32_t ctrl;
    uint32_t cmp;
};

typedef struct apb_timer {
    volatile struct apb_timer_map *apb_timer_map;
    uint64_t time_h;
} apb_timer_t;

static UNUSED timer_properties_t apb_timer_properties = {
    .upcounter = true,
    .bit_width = 32,
    .irqs = 4,
    .periodic_timeouts = true,
    .relative_timeouts = true,
    .absolute_timeouts = false,
    .timeouts = true,
};

int apb_timer_start(apb_timer_t *apb_timer);
int apb_timer_stop(apb_timer_t *apb_timer);
void apb_timer_handle_irq(apb_timer_t *apb_timer, uint32_t irq);
uint64_t apb_timer_get_time(apb_timer_t *apb_timer);
int apb_timer_set_timeout(apb_timer_t *apb_timer, uint64_t ns, bool periodic);
int apb_timer_init(apb_timer_t *apb_timer, apb_timer_config_t config);
