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
    // ZF_LOGD("apb_timer_start() called \n");
    apb_timer->apb_timer_map->ctrl |= APB_TIMER_CTRL_ENABLE;
    // apb_timer->apb_timer_map->ctrl |= (0x7 << 3) | APB_TIMER_CTRL_ENABLE;
    return 0;
}

int apb_timer_stop(apb_timer_t *apb_timer)
{
    /* Disable timer. */
    // ZF_LOGD("apb_timer_stop() called \n");
    apb_timer->apb_timer_map->ctrl &= ~APB_TIMER_CTRL_ENABLE;
    apb_timer->apb_timer_map->cmp = 0;
    apb_timer->apb_timer_map->time = 0;

    return 0;
}

int apb_timer_set_timeout(apb_timer_t *apb_timer, uint64_t ns, bool periodic)
{
    // ZF_LOGD("apb_timer_set_timeout() called \n");
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
    ZF_LOGD("apb_timer->apb_timer_map->cmp = %u and time = %u", apb_timer->apb_timer_map->cmp, apb_timer->apb_timer_map->time);
    apb_timer_start(apb_timer);
    ZF_LOGD("apb_timer->apb_timer_map->cmp = %u and time = %u", apb_timer->apb_timer_map->cmp, apb_timer->apb_timer_map->time);

    if (periodic) {
        // TODO
    } else {
        // TODO
    }

    return 0;
}

void apb_timer_handle_irq(apb_timer_t *apb_timer, uint32_t irq)
{
    ZF_LOGD("apb_timer_handle_irq() called with irq = %u \n", irq);
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
    // int count = 0;

    // for (;;) {
    //     uint64_t time_h1 = (volatile uint64_t) apb_timer->time_h;
    //     uint32_t time1 = apb_timer->apb_timer_map->time;
    //     uint64_t time_h2 = (volatile uint64_t) apb_timer->time_h;
    //     uint32_t time2 = apb_timer->apb_timer_map->time;

    //     if((time_h1 == time_h2) && (time2 > time1)) {
    //         ZF_LOGD("time1 vs time2 -> %u  <-> %u, count = %d, time_h2 = %" PRIu64, time1, time2, count, time_h2);
    //         uint64_t time_temp = (time2 + (time_h2 * 0xFFFFFFFF)) * (NS_IN_S / APB_TIMER_INPUT_FREQ);
    //         ZF_LOGD("time_temp = %" PRIu64, time_temp);
    //         return time_temp;
    //     }

    //     count++;
    // }

            uint64_t time_h2 = (volatile uint64_t) apb_timer->time_h;
            uint32_t time2 = apb_timer->apb_timer_map->time;
            uint64_t time_temp = (time2 + (time_h2 * 0xFFFFFFFF)) * (NS_IN_S / APB_TIMER_INPUT_FREQ);
            ZF_LOGD("time_temp = %" PRIu64, time_temp);
            return time_temp;
}

int apb_timer_init(apb_timer_t *apb_timer, apb_timer_config_t config)
{
    apb_timer->apb_timer_map = (volatile struct apb_timer_map *) config.vaddr;
    apb_timer->time_h = 0;
    return 0;
}
