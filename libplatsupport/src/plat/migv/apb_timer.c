/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2019-2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/apb_timer.h>

#include "../../ltimer.h"

/* Largest timeout that can be programmed */
#define MAX_TIMEOUT_NS (BIT(32) * (NS_IN_S / APB_TIMER_INPUT_FREQ))


int apb_timer_start(apb_timer_t *apb_timer)
{
    apb_timer->apb_timer_map->ctrl |= APB_TIMER_CTRL_ENABLE;

    return 0;
}

int apb_timer_stop(apb_timer_t *apb_timer)
{
    /* Disable timer. */
    apb_timer->apb_timer_map->ctrl &= ~APB_TIMER_CTRL_ENABLE;
    apb_timer->apb_timer_map->cmp = 0;
    apb_timer->apb_timer_map->time = 0;

    return 0;
}

int apb_timer_set_timeout(apb_timer_t *apb_timer, uint64_t ns, bool periodic)
{
    if (ns > MAX_TIMEOUT_NS) {
        ZF_LOGE("Cannot program a timeout larger than %ld ns", MAX_TIMEOUT_NS);
        return -1;
    }

    // Clear whatever state the timer is in.
    apb_timer_stop(apb_timer);
    uint64_t cmp64 = ns / (NS_IN_S / APB_TIMER_INPUT_FREQ);

    if((cmp64 >> 32) != 0) {
        ZF_LOGE("cmp exceeds 32-bit range with %" PRIu64, cmp64);
        return -1;
    }

    apb_timer->apb_timer_map->cmp = (uint32_t) cmp64;
    apb_timer_start(apb_timer);

    return 0;
}

void apb_timer_handle_irq(apb_timer_t *apb_timer, uint32_t irq)
{
    switch(irq) {
        case APB_TIMER_IRQ_OVF(0):
        case APB_TIMER_IRQ_OVF(1):
            apb_timer->time_h++;
            break;
        default:
            break;
    }
}

uint64_t apb_timer_get_time(apb_timer_t *apb_timer)
{
    uint64_t time_h = (volatile uint64_t) apb_timer->time_h;
    uint32_t time = apb_timer->apb_timer_map->time;

    return (uint64_t)((time + (time_h * 0xFFFFFFFF)) * (NS_IN_S / APB_TIMER_INPUT_FREQ));
}

int apb_timer_init(apb_timer_t *apb_timer, apb_timer_config_t config)
{
    apb_timer->apb_timer_map = (volatile struct apb_timer_map *) config.vaddr;
    apb_timer->time_h = 0;

    return 0;
}
