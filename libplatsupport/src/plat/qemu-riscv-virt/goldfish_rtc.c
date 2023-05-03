/*
 * Copyright 2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>

#include <platsupport/timer.h>
#include <platsupport/plat/goldfish_rtc.h>

#include "../../ltimer.h"

/* Largest timeout that can be programmed */
// #define MAX_TIMEOUT_NS (BIT(64) / NS_IN_S)

int goldfish_rtc_alarm_irq_enable(goldfish_rtc_t *goldfish_rtc, bool enabled)
{
    if(enabled)
        goldfish_rtc->goldfish_rtc_map->timer_irq_enabled = 1;
    else
        goldfish_rtc->goldfish_rtc_map->timer_irq_enabled = 0;

    return 0;
}


int goldfish_rtc_start(goldfish_rtc_t *goldfish_rtc)
{
    goldfish_rtc_alarm_irq_enable(goldfish_rtc, true);

    return 0;
}

int goldfish_rtc_stop(goldfish_rtc_t *goldfish_rtc)
{
    /* Disable timer. */
    goldfish_rtc_alarm_irq_enable(goldfish_rtc, false);

    return 0;
}

int goldfish_rtc_init(goldfish_rtc_t *goldfish_rtc, goldfish_rtc_config_t config)
{
    goldfish_rtc->goldfish_rtc_map = (volatile struct goldfish_rtc_map *) config.vaddr;

    goldfish_rtc_set_time(goldfish_rtc, 0);

    return 0;
}

void goldfish_rtc_handle_irq(goldfish_rtc_t *goldfish_rtc, uint32_t irq)
{
    goldfish_rtc->goldfish_rtc_map->timer_clear_interrupt = 1;
}

uint64_t goldfish_rtc_get_time(goldfish_rtc_t *goldfish_rtc)
{
    uint64_t time_low = (volatile uint64_t) goldfish_rtc->goldfish_rtc_map->timer_time_low;
    uint64_t time_high = (volatile uint64_t) goldfish_rtc->goldfish_rtc_map->timer_time_high;
    uint64_t time = (time_high << 32) | time_low;

    return time;
}

int goldfish_rtc_set_time(goldfish_rtc_t *goldfish_rtc, uint64_t ns)
{
    goldfish_rtc->goldfish_rtc_map->timer_time_high = (ns >> 32);
    goldfish_rtc->goldfish_rtc_map->timer_time_low = ns;
}

int goldfish_rtc_set_timeout(goldfish_rtc_t *goldfish_rtc, uint64_t ns, bool periodic)
{
    goldfish_rtc->goldfish_rtc_map->timer_alarm_high = (uint32_t) (ns >> 32);
    goldfish_rtc->goldfish_rtc_map->timer_alarm_low = (uint32_t) ns;

    uint64_t time_low = goldfish_rtc->goldfish_rtc_map->timer_alarm_low;
    uint64_t time_high = goldfish_rtc->goldfish_rtc_map->timer_alarm_high;

    uint64_t time = (time_high << 32) | time_low;

    goldfish_rtc_start(goldfish_rtc);

    return 0;
}