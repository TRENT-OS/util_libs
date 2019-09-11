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
#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>

#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>
#include <platsupport/io.h>

/*
 * We use two hcsc1 timers: one to keep track of an absolute time, the other for timeouts.
 */
#define HCSC1_ID            HCSC1_TIMER0
#define TIMEOUT_HCSC1       0
#define TIMESTAMP_HCSC1     1
#define NUM_HCSC1_TIMERS    2

typedef struct {
    hcsc1_t hcsc1s[NUM_HCSC1_TIMERS];
    void *hcsc1_timer0_vaddr;
    void *hcsc1_timer1_vaddr;
    ps_io_ops_t ops;
    uint32_t high_bits;
} hcsc1_ltimer_t;

static size_t get_num_irqs(void *data)
{
    return 2;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    irq->type = PS_INTERRUPT;
    irq->irq.number = hcsc1_get_irq(HCSC1_ID + n);
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return 2;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    region->length = PAGE_SIZE_4K;
    region->base_addr = (uintptr_t) hcsc1_get_paddr(n);
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    hcsc1_ltimer_t *hcsc1_ltimer = data;
    assert(data != NULL);
    assert(time != NULL);

    hcsc1_t *dualtimer = &hcsc1_ltimer->hcsc1s[TIMESTAMP_HCSC1];
    uint64_t low_ticks = hcsc1_get_ticks(dualtimer);
    uint64_t ticks = hcsc1_ltimer->high_bits + !!hcsc1_is_irq_pending(dualtimer);
    ticks = (ticks << 32llu) + low_ticks;
    *time = hcsc1_ticks_to_ns(ticks);
    return 0;
}

int handle_irq(void *data, ps_irq_t *irq)
{
    hcsc1_ltimer_t *hcsc1_ltimer = data;
    if (irq->irq.number == hcsc1_get_irq(HCSC1_ID + TIMEOUT_HCSC1)) {
        hcsc1_handle_irq(&hcsc1_ltimer->hcsc1s[TIMEOUT_HCSC1]);
    } else if (irq->irq.number == hcsc1_get_irq(HCSC1_ID + TIMESTAMP_HCSC1)) {
        hcsc1_handle_irq(&hcsc1_ltimer->hcsc1s[TIMESTAMP_HCSC1]);
        hcsc1_ltimer->high_bits++;
    } else {
        ZF_LOGE("unknown irq");
        return EINVAL;
    }
    return 0;
}

int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t time;
        int error = get_time(data, &time);
        if (error) {
            return error;
        }
        if (time > ns) {
            return ETIME;
        }
        ns -= time;
    }

    hcsc1_ltimer_t *hcsc1_ltimer = data;
    return hcsc1_set_timeout(&hcsc1_ltimer->hcsc1s[TIMEOUT_HCSC1], ns,
            type == TIMEOUT_PERIODIC);
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int reset(void *data)
{
    hcsc1_ltimer_t *hcsc1_ltimer = data;

    // The seven timer driver tests are passing even if the stop and start below is disabled.
    // From the assumed interface semantics it seems natural to stop and start the timer.

    /* restart the rtc */
    hcsc1_stop(&hcsc1_ltimer->hcsc1s[TIMEOUT_HCSC1]);
    hcsc1_start(&hcsc1_ltimer->hcsc1s[TIMEOUT_HCSC1]);
    return 0;
}

static void destroy(void *data)
{
    pmem_region_t region;
    UNUSED int error;
    hcsc1_ltimer_t *hcsc1_ltimer = data;

    if (hcsc1_ltimer->hcsc1_timer0_vaddr) {
        hcsc1_stop(&hcsc1_ltimer->hcsc1s[TIMEOUT_HCSC1]);
        error = get_nth_pmem(data, 0,  &region);
        assert(!error);
        ps_pmem_unmap(&hcsc1_ltimer->ops, region, hcsc1_ltimer->hcsc1_timer0_vaddr);
    }

    if (hcsc1_ltimer->hcsc1_timer1_vaddr) {
        hcsc1_stop(&hcsc1_ltimer->hcsc1s[TIMESTAMP_HCSC1]);
        error = get_nth_pmem(data, 1,  &region);
        assert(!error);
        ps_pmem_unmap(&hcsc1_ltimer->ops, region, hcsc1_ltimer->hcsc1_timer1_vaddr);
    }
    ps_free(&hcsc1_ltimer->ops.malloc_ops, sizeof(hcsc1_ltimer), hcsc1_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    ltimer_default_describe(ltimer, ops);
    ltimer->handle_irq = handle_irq;
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(hcsc1_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    hcsc1_ltimer_t *hcsc1_ltimer = ltimer->data;
    hcsc1_ltimer->ops = ops;

    /* map the frame for the hcsc1 timers */
    pmem_region_t region;
    error = get_nth_pmem(NULL, 0, &region);
    assert(error == 0);
    hcsc1_ltimer->hcsc1_timer0_vaddr = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (hcsc1_ltimer->hcsc1_timer0_vaddr == NULL) {
        error = ENOMEM;
    }

    /* set up an HCSC1 for timeouts */
    hcsc1_config_t hcsc1_config = {
        .vaddr = hcsc1_ltimer->hcsc1_timer0_vaddr,
        .id = HCSC1_ID
    };

    if (!error) {
        error = hcsc1_init(&hcsc1_ltimer->hcsc1s[TIMEOUT_HCSC1], hcsc1_config);
    }
    if (!error) {
        hcsc1_start(&hcsc1_ltimer->hcsc1s[TIMEOUT_HCSC1]);
    }

    /* set up a HCSC1_TIMER for timestamps */
    hcsc1_config.id++;
    error = get_nth_pmem(NULL, 1, &region);
    assert(error == 0);

    hcsc1_ltimer->hcsc1_timer1_vaddr = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (hcsc1_ltimer->hcsc1_timer1_vaddr == NULL) {
        error = ENOMEM;
    }

    hcsc1_config.vaddr = hcsc1_ltimer->hcsc1_timer1_vaddr;

    if (!error) {
        error = hcsc1_init(&hcsc1_ltimer->hcsc1s[TIMESTAMP_HCSC1], hcsc1_config);
    }
    if (!error) {
        hcsc1_start(&hcsc1_ltimer->hcsc1s[TIMESTAMP_HCSC1]);
    }
    if (!error) {
        error = hcsc1_set_timeout_ticks(&hcsc1_ltimer->hcsc1s[TIMESTAMP_HCSC1], UINT32_MAX, true);
    }

    /* if there was an error, clean up */
    if (error) {
        destroy(ltimer);
    }
    return error;
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
