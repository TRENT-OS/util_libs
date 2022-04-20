/*
 * Copyright (C) 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/plat/rtc.h>

/* RTC Control Register */
#define RTC_PL031_RTCCR_ENABLE_POS         0x0U
#define RTC_PL031_RTCCR_ENABLE_MSK         (0x1U << RTC_PL031_RTCCR_ENABLE_POS)

/* RTC Interrupt Mask Set or Clear Register */
#define RTC_PL031_RTCIMSC_SET_CLEAR_POS    0x0U
#define RTC_PL031_RTCIMSC_SET_CLEAR_MSK    (0x1U << \
                                           RTC_PL031_RTCIMSC_SET_CLEAR_POS)

/* RTC Interrupt Clear Register */
#define RTC_PL031_RTCICR_CLEAR_POS         0x0U
#define RTC_PL031_RTCICR_CLEAR_MSK         (0x1U << RTC_PL031_RTCICR_CLEAR_POS)

int rtc_alarm_irq_enable(rtc_t *rtc, bool enabled)
{
    rtc->rtc_regs->rtcicr = RTC_PL031_RTCICR_CLEAR_MSK;

    if(enabled)
        rtc->rtc_regs->rtcimsc = RTC_PL031_RTCIMSC_SET_CLEAR_MSK;
    else
        rtc->rtc_regs->rtcimsc = 0;
    return 0;
}

int rtc_start(rtc_t *rtc)
{
    // Enable the RTC
    rtc->rtc_regs->rtccr = RTC_PL031_RTCCR_ENABLE_MSK;
    rtc_alarm_irq_enable(rtc, true);
    return 0;
}

int rtc_stop(rtc_t *rtc)
{
    // Disable the RTC
    rtc->rtc_regs->rtccr = 0;
    rtc_alarm_irq_enable(rtc, false);
    // Clear the load and match register
    rtc->rtc_regs->rtclr = 0;
    rtc->rtc_regs->rtcmr = 0;
    return 0;
}

int rtc_init(rtc_t *rtc, rtc_config_t config)
{
    if (config.vaddr == NULL) {
        ZF_LOGE("Vaddr of the mapped RTC device register frame is required.");
        return EINVAL;
    }

    rtc->rtc_regs = (rtc_regs_t *) config.vaddr;
    // Clear the load and match register
    rtc->rtc_regs->rtclr = 0;
    rtc->rtc_regs->rtcmr = 0;
    return 0;
}

void rtc_handle_irq(rtc_t *rtc, uint32_t irq)
{
    // Clear the interrupt
    rtc->rtc_regs->rtcicr = RTC_PL031_RTCICR_CLEAR_MSK;
}

// Return current time (converted from seconds) in nanoseconds
uint64_t rtc_get_time(rtc_t *rtc)
{
    uint64_t ticks = rtc->rtc_regs->rtcdr;
    return ticks * NS_IN_S;
}

// Set timeout in seconds
int rtc_set_timeout(rtc_t *rtc, uint32_t timeout, bool periodic)
{
    rtc->rtc_regs->rtcmr = timeout;
    return 0;
}