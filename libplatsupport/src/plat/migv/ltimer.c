/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2019-2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Implementation of a logical timer for MiG-V SoC platform.
 */

#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/pmem.h>
#include <utils/util.h>
#include <platsupport/plat/apb_timer.h>

#include "../../ltimer.h"

enum {
    COUNTER_TIMER, /* Use APB_TIMER0 to keep running for timestamp/gettime */
    TIMEOUT_TIMER, /* Use APB_TIMER1 for timeouts/sleep */
    NUM_TIMERS
};

typedef struct {
    apb_timer_t apb_timer;
    void *vaddr;
} apb_timer_ltimer_t;

typedef struct {
    apb_timer_ltimer_t apb_timer_ltimers[NUM_TIMERS];
    irq_id_t timer_irq_ids[NUM_TIMERS];
    timer_callback_data_t callback_datas[NUM_TIMERS];
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
} migv_ltimers_t;

static ps_irq_t irqs[] = {
    {
        .type = PS_INTERRUPT,
        .irq.number = APB_TIMER_IRQ_OVF(0)
    },
    {
        .type = PS_INTERRUPT,
        .irq.number = APB_TIMER_IRQ_CMP(1)
    },
};

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = APB_TIMER0_PADDR,
        .length = PAGE_SIZE_4K
    },
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = APB_TIMER1_PADDR,
        .length = PAGE_SIZE_4K
    },
};

#define N_IRQS ARRAY_SIZE(irqs)
#define N_PMEMS ARRAY_SIZE(pmems)

size_t get_num_irqs(void *data)
{
    return N_IRQS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < N_IRQS);

    *irq = irqs[n];

    return 0;
}

static size_t get_num_pmems(void *data)
{
    return N_PMEMS;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    assert(n < N_PMEMS);
    *paddr = pmems[n];

    return 0;
}

static int ltimer_handle_irq(void *data, ps_irq_t *irq)
{
    // ZF_LOGD("handle_irq() called \n");
    assert(data != NULL);
    migv_ltimers_t *timers = data;
    long irq_number = irq->irq.number;
    ltimer_event_t event;

    switch(irq_number) {
        case APB_TIMER_IRQ_OVF(0):
        case APB_TIMER_IRQ_OVF(1):
            apb_timer_handle_irq(&timers->apb_timer_ltimers[COUNTER_TIMER].apb_timer, irq_number);
            event = LTIMER_OVERFLOW_EVENT;
            break;
        case APB_TIMER_IRQ_CMP(1):
            apb_timer_stop(&timers->apb_timer_ltimers[TIMEOUT_TIMER].apb_timer);
            event = LTIMER_TIMEOUT_EVENT;
            break;
        case APB_TIMER_IRQ_CMP(0):
            event = LTIMER_TIMEOUT_EVENT;
            break;
        default:
            ZF_LOGE("Invalid IRQ number: %d received.", irq_number);
            return 0; // TODO: return -1 ?
    }

    if (timers->user_callback) {
        timers->user_callback(timers->user_callback_token, event);
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);
    migv_ltimers_t *timers = data;

    *time = apb_timer_get_time(&timers->apb_timer_ltimers[COUNTER_TIMER].apb_timer);

    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    migv_ltimers_t *timers = data;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = apb_timer_get_time(&timers->apb_timer_ltimers[COUNTER_TIMER].apb_timer);
        if (time >= ns) {
            return ETIME;
        }
        apb_timer_set_timeout(&timers->apb_timer_ltimers[TIMEOUT_TIMER].apb_timer, ns - time, false);
        return 0;
    }
    case TIMEOUT_RELATIVE: {
        return apb_timer_set_timeout(&timers->apb_timer_ltimers[TIMEOUT_TIMER].apb_timer, ns, false);
    }
    case TIMEOUT_PERIODIC: {
        return apb_timer_set_timeout(&timers->apb_timer_ltimers[TIMEOUT_TIMER].apb_timer, ns, true);
    }
    default:
        break;
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    migv_ltimers_t *timers = data;
    apb_timer_stop(&timers->apb_timer_ltimers[COUNTER_TIMER].apb_timer);
    apb_timer_start(&timers->apb_timer_ltimers[COUNTER_TIMER].apb_timer);
    apb_timer_stop(&timers->apb_timer_ltimers[TIMEOUT_TIMER].apb_timer);
    apb_timer_start(&timers->apb_timer_ltimers[TIMEOUT_TIMER].apb_timer);

    return 0;
}

static void destroy(void *data)
{
    assert(data);
    migv_ltimers_t *timers = data;

    for (int i = 0; i < NUM_TIMERS; i++) {
        if (timers->apb_timer_ltimers[i].vaddr) {
            apb_timer_stop(&timers->apb_timer_ltimers[i].apb_timer);
            ps_pmem_unmap(&timers->ops, pmems[i], timers->apb_timer_ltimers[i].vaddr);
        }
        if (timers->timer_irq_ids[i] > PS_INVALID_IRQ_ID) {
            int error = ps_irq_unregister(&timers->ops.irq_ops, timers->timer_irq_ids[i]);
            ZF_LOGF_IF(error, "Failed to unregister IRQ");
        }
    }

    ps_free(&timers->ops.malloc_ops, sizeof(timers), timers);
}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{
    assert(ltimer != NULL);
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(migv_ltimers_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    return 0;
}

static int init_ltimer(ltimer_t *ltimer)
{
    assert(ltimer != NULL);
    migv_ltimers_t *timers = ltimer->data;

    /* setup apb_timer */
    apb_timer_config_t config_counter = {
        .vaddr = timers->apb_timer_ltimers[COUNTER_TIMER].vaddr
    };
    apb_timer_config_t config_timeout = {
        .vaddr = timers->apb_timer_ltimers[TIMEOUT_TIMER].vaddr
    };

    apb_timer_init(&timers->apb_timer_ltimers[COUNTER_TIMER].apb_timer, config_counter);
    apb_timer_init(&timers->apb_timer_ltimers[TIMEOUT_TIMER].apb_timer, config_timeout);
    apb_timer_start(&timers->apb_timer_ltimers[COUNTER_TIMER].apb_timer);

    return 0;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    error = create_ltimer(ltimer, ops);
    if (error) {
        return error;
    }

    migv_ltimers_t *timers = ltimer->data;
    timers->ops = ops;
    timers->user_callback = callback;
    timers->user_callback_token = callback_token;
    for (int i = 0; i < NUM_TIMERS; i++) {
        timers->timer_irq_ids[i] = PS_INVALID_IRQ_ID;
    }

    for (int i = 0; i < NUM_TIMERS; i++) {
        /* map the registers we need */
        timers->apb_timer_ltimers[i].vaddr = ps_pmem_map(&ops, pmems[i], false, PS_MEM_NORMAL);
        if (timers->apb_timer_ltimers[i].vaddr == NULL) {
            destroy(ltimer->data);
            return ENOMEM;
        }

        /* register the IRQs we need */
        timers->callback_datas[i].ltimer = ltimer;
        timers->callback_datas[i].irq = &irqs[i];
        timers->callback_datas[i].irq_handler = ltimer_handle_irq;
        timers->timer_irq_ids[i] = ps_irq_register(&ops.irq_ops, irqs[i], handle_irq_wrapper,
                                                   &timers->callback_datas[i]);
        if (timers->timer_irq_ids[i] < 0) {
            destroy(ltimer->data);
            return EIO;
        }
    }

    error = init_ltimer(ltimer);
    if (error) {
        destroy(ltimer->data);
        return error;
    }

    /* success! */
    return 0;
}

int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("Timer is NULL!");
        return EINVAL;
    }

    ltimer->get_num_irqs = get_num_irqs;
    ltimer->get_nth_irq = get_nth_irq;
    ltimer->get_num_pmems = get_num_pmems;
    ltimer->get_nth_pmem = get_nth_pmem;

    return 0;
}
