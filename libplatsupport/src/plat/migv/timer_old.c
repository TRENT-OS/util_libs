/*
 * Copyright 2020-2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>

#include <utils/util.h>
#include <utils/time.h>
#include <platsupport/plat/timer_old.h>


//#define EMPTY

#define TIMER_ENABLE   BIT(0)

// We are running with the system clock - 20 MHz on the fpga board.
#define TICKS_PER_SECOND 20000000
#define TICKS_PER_MS    (TICKS_PER_SECOND / MS_IN_S)

typedef volatile struct hcsc1_regs {
	uint32_t currentTimer;
	uint32_t timerControl;
	uint32_t timerCompare;
} hcsc1_regs_t;

static hcsc1_regs_t *get_regs(hcsc1_t *hcsc1)
{
    return hcsc1->regs;
}

static void hcsc1_timer_reset(hcsc1_t *hcsc1)
{
#if defined(EMPTY)
    (void)hcsc1;
    return;
#endif

    hcsc1_regs_t *hcsc1_regs = get_regs(hcsc1);

    /* Disable; pre-scaler of zero. */
    hcsc1_regs->timerControl = 0;

    hcsc1->singleShotTimeoutIsActive = false;
}

int hcsc1_stop(hcsc1_t *hcsc1)
{
#if defined(EMPTY)
    (void)hcsc1;
    return 0;
#endif

    if (hcsc1 == NULL) {
        return EINVAL;
    }
    hcsc1_regs_t *hcsc1_regs = get_regs(hcsc1);
    hcsc1_regs->timerControl = hcsc1_regs->timerControl & ~TIMER_ENABLE;
    return 0;
}

int hcsc1_start(hcsc1_t *hcsc1)
{
#if defined(EMPTY)
    (void)hcsc1;
    return 0;
#endif

    if (hcsc1 == NULL) {
        return EINVAL;
    }
    hcsc1_regs_t *hcsc1_regs = get_regs(hcsc1);
    hcsc1_regs->timerControl = hcsc1_regs->timerControl | TIMER_ENABLE;
    return 0;
}

uint64_t hcsc1_ticks_to_ns(uint64_t ticks) {
#if defined(EMPTY)
    (void)ticks;
    return 0;
#endif
    return ticks / TICKS_PER_MS * NS_IN_MS;
}

bool hcsc1_is_irq_pending(hcsc1_t *hcsc1) {
#if defined(EMPTY)
    (void)hcsc1;
    return 0;
#endif

    if (hcsc1) {
       return false;
    }
    return false;
}

int hcsc1_set_timeout(hcsc1_t *hcsc1, uint64_t ns, bool periodic)
{
#if defined(EMPTY)
    (void)hcsc1;
    (void)ns;
    (void)periodic;
    return 0;
#endif
    uint64_t ticks64 = ns * TICKS_PER_MS / NS_IN_MS;
    if (ticks64 > UINT32_MAX) {
        return ETIME;
    }
    return hcsc1_set_timeout_ticks(hcsc1, ticks64, periodic);
}

int hcsc1_set_timeout_ticks(hcsc1_t *hcsc1, uint32_t ticks, bool periodic)
{
#if defined(EMPTY)
    (void)hcsc1;
    (void)ticks;
    (void)periodic;
    return 0;
#endif
    if (hcsc1 == NULL) {
        return EINVAL;
    }

    if (!periodic)
    {
        hcsc1->singleShotTimeoutIsActive = true;
    }

    // Set the timer value
    hcsc1_regs_t *hcsc1_regs = get_regs(hcsc1);
    hcsc1_regs->timerCompare = ticks;

    // Enable the timer interrupt
    hcsc1_stop(hcsc1);
    hcsc1_start(hcsc1);
    return 0;
}

void hcsc1_handle_irq(hcsc1_t *hcsc1)
{
#if defined(EMPTY)
    (void)hcsc1;
    return;
#endif

    if (hcsc1 == NULL) {
        return;
    }

    if (hcsc1->singleShotTimeoutIsActive)
    {
        hcsc1->singleShotTimeoutIsActive = false;
        hcsc1_stop(hcsc1);
    }
}

uint64_t hcsc1_get_ticks(hcsc1_t *hcsc1)
{
#if defined(EMPTY)
    (void)hcsc1;
    return 0;
#endif

    if (hcsc1 == NULL) {
        return 0;
    }
    hcsc1_regs_t *hcsc1_regs = get_regs(hcsc1);
    return hcsc1_regs->currentTimer;
}

uint64_t hcsc1_get_time(hcsc1_t *hcsc1)
{
#if defined(EMPTY)
    (void)hcsc1;
    return 0;
#endif
    return hcsc1_ticks_to_ns(hcsc1_get_ticks(hcsc1));
}

int hcsc1_init(hcsc1_t *hcsc1, hcsc1_config_t config)
{
#if defined(EMPTY)
    (void)hcsc1;
    (void)config;
    return 0;
#endif

    if (config.id > HCSC1_TIMER1) {
        ZF_LOGE("Invalid timer device ID for a hikey dual-timer.");
        return EINVAL;
    }

    if (config.vaddr == NULL || hcsc1 == NULL) {
        ZF_LOGE("Vaddr for the mapped dual-timer device register frame is"
                " required.");
        return EINVAL;
    }

    hcsc1->regs = config.vaddr;

    hcsc1_timer_reset(hcsc1);
    return 0;
}
