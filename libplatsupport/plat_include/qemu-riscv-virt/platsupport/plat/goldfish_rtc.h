/*
 * Copyright 2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/timer.h>

#include <utils/util.h>
#include <stdint.h>
#include <stdbool.h>

#define GOLDFISH_RTC_PADDR   0x101000
#define GOLDFISH_RTC_IRQ   0x0b

typedef struct {
    /* vaddr goldfish_rtc is mapped to */
    void *vaddr;
} goldfish_rtc_config_t;

/* Memory map for goldfish rtc */
struct goldfish_rtc_map {
    uint32_t timer_time_low; /* get low bits of current time and update TIMER_TIME_HIGH */
    uint32_t timer_time_high; /* get high bits of time at last TIMER_TIME_LOW read */
    uint32_t timer_alarm_low; /* set low bits of alarm and activate it */
    uint32_t timer_alarm_high; /* set high bits of next alarm */
    uint32_t timer_irq_enabled;
    uint32_t timer_clear_alarm;
    uint32_t timer_alarm_status;
    uint32_t timer_clear_interrupt;
};

typedef struct goldfish_rtc {
    volatile struct goldfish_rtc_map *goldfish_rtc_map;
    uint64_t time_h;
} goldfish_rtc_t;

int goldfish_rtc_start(goldfish_rtc_t *apb_timer);
int goldfish_rtc_stop(goldfish_rtc_t *apb_timer);
void goldfish_rtc_handle_irq(goldfish_rtc_t *apb_timer, uint32_t irq);
uint64_t goldfish_rtc_get_time(goldfish_rtc_t *apb_timer);
int goldfish_rtc_set_time(goldfish_rtc_t *apb_timer, uint64_t ns);
int goldfish_rtc_set_timeout(goldfish_rtc_t *apb_timer, uint64_t ns, bool periodic);
int goldfish_rtc_init(goldfish_rtc_t *apb_timer, goldfish_rtc_config_t config);