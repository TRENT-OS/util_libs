/*
 * Copyright (C) 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/ltimer.h>
#include <platsupport/io.h>
#include <utils/io.h>
#include <platsupport/plat/rtc.h>

#include "../../ltimer.h"

enum {
    RTC = 0,
    NUM_TIMERS = 1
};

typedef struct {
    rtc_t rtc;
    void *rtc_vaddr;
    irq_id_t timer_irq_id;
    timer_callback_data_t callback_data;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
} rtc_ltimer_t;

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = RTC_PADDR,
        .length = PAGE_SIZE_4K
    }
};

static ps_irq_t irqs[] = {
    {
        .type = PS_INTERRUPT,
        .irq.number = RTC_IRQ
    }
};

static size_t get_num_irqs(void *data)
{
    return sizeof(irqs) / sizeof(irqs[0]);
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    *irq = irqs[n];
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return sizeof(pmems) / sizeof(pmems[0]);
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    assert(n < get_num_pmems(data));
    *region = pmems[n];
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    rtc_ltimer_t *rtc = data;

    if (irq->irq.number != RTC_IRQ) {
        ZF_LOGE("Got IRQ from unkown source?");
        return EINVAL;
    }

    if (rtc->user_callback) {
        /* The only interrupt event that we'll get is a timeout event */
        rtc->user_callback(rtc->user_callback_token, LTIMER_TIMEOUT_EVENT);
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(time);
    assert(data);

    rtc_ltimer_t *rtc = data;
    *time = rtc_get_time(&rtc->rtc);

    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    rtc_ltimer_t *rtc = data;
    bool periodic = false;
    uint32_t timeout;
    uint64_t current_time;

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        current_time = rtc_get_time(&rtc->rtc);
        if (current_time > ns) {
            ZF_LOGE("Timeout in the past: now %"PRIu64", timeout %"PRIu64, current_time, ns);
            return ETIME;
        } else if ((ns - current_time) / NS_IN_S > UINT32_MAX) {
            ZF_LOGE("Timeout too far in the future");
            return ETIME;
        }

        timeout = (ns - current_time) / NS_IN_S;
        break;
    case TIMEOUT_PERIODIC:
        periodic = true;
    /* Fall through */
    case TIMEOUT_RELATIVE:
        if (ns / NS_IN_S > UINT32_MAX) {
            ZF_LOGE("Timeout too far in the future");
            return ETIME;
        }

        timeout = ns / NS_IN_S;
        break;
    }

    rtc_set_timeout(&rtc->rtc, timeout, periodic);

    return 0;
}

static int reset(void *data)
{
    assert(data != NULL);
    rtc_ltimer_t *rtc = data;

    rtc_stop(&rtc->rtc);
    rtc_start(&rtc->rtc);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    rtc_ltimer_t *rtc = data;

    rtc_stop(&rtc->rtc);

    if (rtc->rtc_vaddr) {
        ps_pmem_unmap(&rtc->ops, pmems[0], rtc->rtc_vaddr);
    }

    if (rtc->timer_irq_id > PS_INVALID_IRQ_ID) {
        int error = ps_irq_unregister(&rtc->ops.irq_ops, rtc->timer_irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }

    ps_free(&rtc->ops.malloc_ops, sizeof(rtc_ltimer_t), rtc);
    return;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(rtc_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    rtc_ltimer_t *rtc = ltimer->data;

    rtc->ops = ops;
    rtc->user_callback = callback;
    rtc->user_callback_token = callback_token;
    rtc->timer_irq_id = PS_INVALID_IRQ_ID;

    rtc->rtc_vaddr = ps_pmem_map(&ops, pmems[0], false, PS_MEM_NORMAL);
    if (rtc->rtc_vaddr == NULL) {
        destroy(ltimer->data);
        return EINVAL;
    }

    rtc->callback_data.ltimer = ltimer;
    rtc->callback_data.irq_handler = handle_irq;
    rtc->callback_data.irq = &irqs[0];

    rtc->timer_irq_id = ps_irq_register(&ops.irq_ops, irqs[0], handle_irq_wrapper,
                                                   &rtc->callback_data);
    if (rtc->timer_irq_id < 0) {
        destroy(ltimer->data);
        return EIO;
    }

    rtc_config_t rtc_config = {
        .vaddr = rtc->rtc_vaddr
    };

    rtc_init(&rtc->rtc, rtc_config);
    rtc_start(&rtc->rtc);
    return 0;
}

int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("Timer is NULL!");
        return EINVAL;
    }

    *ltimer = (ltimer_t) {
        .get_num_irqs = get_num_irqs,
        .get_nth_irq = get_nth_irq,
        .get_num_pmems = get_num_pmems,
        .get_nth_pmem = get_nth_pmem
    };

    return 0;
}