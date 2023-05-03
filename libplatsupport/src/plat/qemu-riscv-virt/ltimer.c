/*
 * Copyright 2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/ltimer.h>
#include <platsupport/io.h>
#include <utils/io.h>
#include <platsupport/plat/goldfish_rtc.h>

#include "../../ltimer.h"

enum {
    GOLDFISH_RTC = 0,
    NUM_TIMERS = 1
};

typedef struct {
    goldfish_rtc_t goldfish_rtc;
    void *goldfish_rtc_vaddr;
    irq_id_t timer_irq_id;
    timer_callback_data_t callback_data;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
} goldfish_rtc_ltimer_t;

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = GOLDFISH_RTC_PADDR,
        .length = PAGE_SIZE_4K
    }
};

static ps_irq_t irqs[] = {
    {
        .type = PS_INTERRUPT,
        .irq.number = GOLDFISH_RTC_IRQ
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
    goldfish_rtc_ltimer_t *goldfish_rtc = data;

    if (irq->irq.number != GOLDFISH_RTC_IRQ) {
        ZF_LOGE("Got IRQ from unkown source?");
        return EINVAL;
    }

    if (goldfish_rtc->user_callback) {
        /* The only interrupt event that we'll get is a timeout event */
        goldfish_rtc->user_callback(goldfish_rtc->user_callback_token, LTIMER_TIMEOUT_EVENT);
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(time);
    assert(data);

    goldfish_rtc_ltimer_t *goldfish_rtc = data;

    *time = goldfish_rtc_get_time(&goldfish_rtc->goldfish_rtc);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    goldfish_rtc_ltimer_t *goldfish_rtc = data;
    bool periodic = false;
    // uint16_t timeout;
    // uint64_t current_time;

    // switch (type) {
    // case TIMEOUT_ABSOLUTE:
    //     current_time = goldfish_rtc_get_time(&goldfish_rtc->goldfish_rtc);
    //     if (current_time > ns) {
    //         ZF_LOGE("Timeout in the past: now %lu, timeout %lu", current_time, ns);
    //         return ETIME;
    //     } else if ((ns - current_time) / NS_IN_MS > UINT16_MAX) {
    //         ZF_LOGE("Timeout too far in the future");
    //         return ETIME;
    //     }

    //     timeout = (ns - current_time) / NS_IN_MS;
    //     break;
    // case TIMEOUT_PERIODIC:
    //     periodic = true;
    // /* Fall through */
    // case TIMEOUT_RELATIVE:
    //     if (ns / NS_IN_MS > UINT16_MAX) {
    //         ZF_LOGE("Timeout too far in the future");
    //         return ETIME;
    //     }

    //     timeout = ns / NS_IN_MS;
    //     break;
    // }

    // /* The timer tick includes 0, i.e. one unit of time greater than the value written */
    // timeout = MIN(timeout, (uint16_t)(timeout - 1));
    goldfish_rtc_set_timeout(&goldfish_rtc->goldfish_rtc, ns, periodic);
    return 0;
}

static int reset(void *data)
{
    assert(data != NULL);
    goldfish_rtc_ltimer_t *goldfish_rtc = data;

    goldfish_rtc_stop(&goldfish_rtc->goldfish_rtc);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    goldfish_rtc_ltimer_t *goldfish_rtc = data;

    goldfish_rtc_stop(&goldfish_rtc->goldfish_rtc);

    if (goldfish_rtc->goldfish_rtc_vaddr) {
        ps_pmem_unmap(&goldfish_rtc->ops, pmems[0], goldfish_rtc->goldfish_rtc_vaddr);
    }

    if (goldfish_rtc->timer_irq_id > PS_INVALID_IRQ_ID) {
        int error = ps_irq_unregister(&goldfish_rtc->ops.irq_ops, goldfish_rtc->timer_irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }

    ps_free(&goldfish_rtc->ops.malloc_ops, sizeof(goldfish_rtc_ltimer_t), goldfish_rtc);
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

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(goldfish_rtc_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    goldfish_rtc_ltimer_t *goldfish_rtc = ltimer->data;

    goldfish_rtc->ops = ops;
    goldfish_rtc->user_callback = callback;
    goldfish_rtc->user_callback_token = callback_token;
    goldfish_rtc->timer_irq_id = PS_INVALID_IRQ_ID;

    goldfish_rtc->goldfish_rtc_vaddr = ps_pmem_map(&ops, pmems[0], false, PS_MEM_NORMAL);
    if (goldfish_rtc->goldfish_rtc_vaddr == NULL) {
        destroy(ltimer->data);
        return EINVAL;
    }

    goldfish_rtc->callback_data.ltimer = ltimer;
    goldfish_rtc->callback_data.irq_handler = handle_irq;
    goldfish_rtc->callback_data.irq = &irqs[0];

    goldfish_rtc->timer_irq_id = ps_irq_register(&ops.irq_ops, irqs[0], handle_irq_wrapper,
                                                   &goldfish_rtc->callback_data);
    if (goldfish_rtc->timer_irq_id < 0) {
        destroy(ltimer->data);
        return EIO;
    }

    goldfish_rtc_config_t goldfish_rtc_config = {
        .vaddr = goldfish_rtc->goldfish_rtc_vaddr
    };
    ZF_LOGE("ltimer_default_init - before goldfish_rtc_init");
    goldfish_rtc_init(&goldfish_rtc->goldfish_rtc, goldfish_rtc_config);
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