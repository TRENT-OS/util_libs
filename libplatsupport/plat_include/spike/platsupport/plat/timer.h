/*
 * Copyright 2019, Data61
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

#include <platsupport/timer.h>

/* We use two hardware timers. */
#define HCSC1_0_PADDR 0x00409000
#define HCSC1_1_PADDR 0x0040a000

/* IRQs */

/*
    Note: for the time being we assume (know) ltimer is using 
    timer 0 as timeout and timer 1 as timestamp. 
*/

/* Timeout */
#define HCSC1_0_INTERRUPT   16

/* Overflow */
#define HCSC1_1_INTERRUPT   17

/* Timers */
typedef enum timer_id {
    HCSC1_TIMER0,
    HCSC1_TIMER1,
    NUM_HCSC1_TIMERS = 2,
} hcsc1_id_t;

#define TMR_DEFAULT DMTIMER0

static const uintptr_t hcsc1_timer_paddrs[] = {
    [HCSC1_TIMER0] = HCSC1_0_PADDR,
    [HCSC1_TIMER1] = HCSC1_1_PADDR,
};

static const long hcsc1_timer_irqs[] = {
    [HCSC1_TIMER0] = HCSC1_0_INTERRUPT,
    [HCSC1_TIMER1] = HCSC1_1_INTERRUPT,
};

static inline void *hcsc1_get_paddr(hcsc1_id_t id)
{
    if (id < NUM_HCSC1_TIMERS && id >= HCSC1_TIMER0) {
        return (void *) hcsc1_timer_paddrs[id];
    }
    return NULL;
}

static inline long hcsc1_get_irq(hcsc1_id_t id)
{
    if (id < NUM_HCSC1_TIMERS && id >= HCSC1_TIMER0) {
        return hcsc1_timer_irqs[id];
    }
    return 0;
}

static UNUSED timer_properties_t hcsc1_timer_props = {
    .upcounter = false,
    .timeouts = true,
    .absolute_timeouts = false,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};

typedef struct {
    void *regs;
    bool singleShotTimeoutIsActive;
} hcsc1_t;

typedef struct {
    void *vaddr;
    int id;
} hcsc1_config_t;

int hcsc1_init(hcsc1_t *timer, hcsc1_config_t config);
void hcsc1_handle_irq(hcsc1_t *timer);
/* convert between dmt ticks and ns */
uint64_t hcsc1_ticks_to_ns(uint64_t ticks);
/* return true if an overflow irq is pending */
bool hcsc1_is_irq_pending(hcsc1_t *hcsc1);
int hcsc1_set_timeout_ticks(hcsc1_t *timer, uint32_t ticks, bool periodic);
/* set a timeout in nano seconds */
int hcsc1_set_timeout(hcsc1_t *timer, uint64_t ns, bool periodic);
int hcsc1_start(hcsc1_t *timer);
int hcsc1_stop(hcsc1_t *timer);
uint64_t hcsc1_get_time(hcsc1_t *timer);
uint64_t hcsc1_get_ticks(hcsc1_t *timer);
