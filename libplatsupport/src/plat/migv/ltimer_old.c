/*
 * Copyright 2019-2021, HENSOLDT Cyber
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */



// TODO: align this code with the ariane code blow to make it work again

#include "../../ltimer.h"
#include <platsupport/timer.h>
#include <platsupport/pmem.h>
#include <utils/util.h>
#include <utils/time.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>

/* FPGA board runs at 20 MHz */
#define TICKS_PER_SECOND    20000000
#define TICKS_PER_MS        (TICKS_PER_SECOND / MS_IN_S)

/* Timer 0 */
#define MIGV_TIMER_0_PADDR          0x00409000
#define MIGV_TIMER_0_IRQ_OVERFLOW   15
#define MIGV_TIMER_0_IRQ_TIMEOUT    16

/* Timer 1 */
#define MIGV_TIMER_1_PADDR          0x0040a000
#define MIGV_TIMER_1_IRQ_OVERFLOW   17
#define MIGV_TIMER_1_IRQ_TIMEOUT    18

/* Timer 2 */
#define MIGV_TIMER_2_PADDR          0x0040b000
#define MIGV_TIMER_2_IRQ_OVERFLOW   19
#define MIGV_TIMER_2_IRQ_TIMEOUT    20


/* Timer registers */
typedef volatile struct {
    uint32_t value;
    uint32_t control;
    uint32_t compare;
} migv_timer_regs_t;


#define TIMER_ENABLE   BIT(0)



/* Timers */
typedef enum  {
    MIGV_TIMER_0,
    MIGV_TIMER_1,

    MIGV_TIMER_TIMEOUT   = MIGV_TIMER_0,
    MIGV_TIMER_TIMESTAMP = MIGV_TIMER_1,

    NUM_MIGV_TIMERS = 2,
} migv_timer_id_t;


#define MIGV_TIMER_TIMEOUT_IRQ      MIGV_TIMER_0_IRQ_TIMEOUT
#define MIGV_TIMER_TIMESTAMP_IRQ    MIGV_TIMER_1_IRQ_OVERFLOW

#define MIGV_TIMER_TIMEOUT_PADDR    MIGV_TIMER_0_PADDR
#define MIGV_TIMER_TIMESTAMP_PADDR  MIGV_TIMER_1_PADDR


typedef struct {
    ltimer_callback_fn_t   func;
    void                   *token;
    timer_callback_data_t  data;
} migv_timer_callback_t;

typedef struct {
    migv_timer_regs_t      *regs;
    bool                   singleShotTimeoutIsActive;
    migv_timer_callback_t  callback;
    irq_id_t               irq_id;
    ps_irq_t               ps_irq;
} migv_hw_timer_t;

typedef struct {
    ps_io_ops_t            ops;
    migv_hw_timer_t        hw_timers[NUM_MIGV_TIMERS];
} migv_timer_ctx_t;


//------------------------------------------------------------------------------
static uint64_t migv_hw_timer_ticks_to_ns(
    uint64_t ticks) {

    return ticks / TICKS_PER_MS * NS_IN_MS;
}


//------------------------------------------------------------------------------
static void migv_hw_timer_reset(
    migv_hw_timer_t *timer)
{
    assert(NULL != timer);

    /* Disable; pre-scaler of zero. */
    timer->regs->control = 0;

    timer->singleShotTimeoutIsActive = false;
}


//------------------------------------------------------------------------------
static void migv_hw_timer_stop(
    migv_hw_timer_t *timer)
{
    assert(NULL != timer);

    // clear enable flag
    timer->regs->control &= ~TIMER_ENABLE;
}


//------------------------------------------------------------------------------
static void migv_hw_timer_start(
    migv_hw_timer_t *timer)
{
    assert(NULL != timer);

    // set enable flag
    timer->regs->control |= TIMER_ENABLE;
}


//------------------------------------------------------------------------------
static uint64_t migv_hw_timer_get_ticks(
    migv_hw_timer_t *timer)
{
    assert(NULL != timer);

    return timer->regs->value;
}


//------------------------------------------------------------------------------
static uint64_t migv_hw_timer_get_time(
    migv_hw_timer_t *timer)
{
    assert(NULL != timer);

    uint64_t ticks = migv_hw_timer_get_ticks(timer);

    return migv_hw_timer_ticks_to_ns(ticks);
}


//------------------------------------------------------------------------------
static void migv_set_timeout_ticks(
    migv_hw_timer_t *timer,
    uint32_t ticks,
    bool periodic)
{
    assert(NULL != timer);

    migv_hw_timer_stop(timer);

    if (!periodic)
    {
        timer->singleShotTimeoutIsActive = true;
    }

    // Set the timer value
    timer->regs->compare = ticks;

    // Enable the timer interrupt
    migv_hw_timer_start(timer);
}


//------------------------------------------------------------------------------
static int migv_hw_timer_set_timeout(
    migv_hw_timer_t *timer,
    uint64_t ns,
    bool periodic)
{
    assert(NULL != timer);

    uint64_t ticks64 = ns * TICKS_PER_MS / NS_IN_MS;

    if (ticks64 >= UINT32_MAX) {
        ZF_LOGE("timeout value too large.");
        return ETIME;
    }

    migv_set_timeout_ticks(timer, ticks64, periodic);

    return 0;
}


//------------------------------------------------------------------------------
static bool migv_hw_timer_is_irq_pending(
    migv_hw_timer_t *timer) {

    assert(NULL != timer);

    // ToDo ....
    return false;
}


//------------------------------------------------------------------------------
static void migv_hw_timer_handle_irq(
    migv_hw_timer_t *timer)
{
    assert(NULL != timer);

    if (timer->singleShotTimeoutIsActive)
    {
        timer->singleShotTimeoutIsActive = false;
        migv_hw_timer_stop(timer);

        if (timer->callback.func)
        {
            timer->callback.func(timer->callback.token, LTIMER_TIMEOUT_EVENT);
        }
    }
}


//------------------------------------------------------------------------------
// ltimer callback
static size_t get_num_irqs(
    void *data)
{
    assert(NULL != data);

    return NUM_MIGV_TIMERS;
}


//------------------------------------------------------------------------------
// ltimer callback
static int get_nth_irq(
    void *data,
    size_t n,
    ps_irq_t *irq)
{
    assert(NULL != data);
    assert(NULL != irq);

    switch(n)
    {
        //---------------------------------------------------
        case MIGV_TIMER_TIMEOUT:
            irq->type = PS_INTERRUPT;
            irq->irq.number = MIGV_TIMER_TIMEOUT_IRQ;
            return 0;

        //---------------------------------------------------
        case MIGV_TIMER_TIMESTAMP:
            irq->type = PS_INTERRUPT;
            irq->irq.number = MIGV_TIMER_TIMESTAMP_IRQ;
            return 0;

        //---------------------------------------------------
        default:
            break;
    };

    ZF_LOGE("invalid n for get_nth_irq(): %zu", n);
    return -1;
}


//------------------------------------------------------------------------------
// ltimer callback
static size_t get_num_pmems(
    void *data)
{
    assert(NULL != data);

    return NUM_MIGV_TIMERS;
}


//------------------------------------------------------------------------------
// ltimer callback
static int get_nth_pmem(
    void *data,
    size_t n,
    pmem_region_t *region)
{
    assert(NULL != data);
    assert(NULL != region);

    switch(n)
    {
        //---------------------------------------------------
        case MIGV_TIMER_TIMEOUT:
            region->length = PAGE_SIZE_4K;
            region->base_addr = (uintptr_t)MIGV_TIMER_TIMEOUT_PADDR;
            return 0;

        //---------------------------------------------------
        case MIGV_TIMER_TIMESTAMP:
            region->length = PAGE_SIZE_4K;
            region->base_addr = (uintptr_t)MIGV_TIMER_TIMESTAMP_PADDR;
            return 0;

        //---------------------------------------------------
        default:
            break;
    };

    ZF_LOGE("invalid n for get_nth_pmem(): %zu", n);
    return -1;
}


//------------------------------------------------------------------------------
static int get_time(
    void *data,
    uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    migv_timer_ctx_t *timer_ctx = data;

    migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMESTAMP]);
    uint64_t ticks = migv_hw_timer_get_ticks(timer);

    // uint64_t ticks = timer->high_bits + !!migv_hw_timer_is_irq_pending(timer);
    // ticks = (ticks << 32llu) + low_ticks;

    *time = migv_hw_timer_ticks_to_ns(ticks);

    return 0;
}


//------------------------------------------------------------------------------
int handle_irq(
    void *data,
    ps_irq_t *irq)
{
    assert(data != NULL);

    migv_timer_ctx_t *timer_ctx = data;

    switch (irq->irq.number)
    {
        //----------------------------------------------
        case MIGV_TIMER_TIMEOUT_IRQ:
        {
            migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMEOUT]);
            migv_hw_timer_handle_irq(timer);
            return 0;
        }

        //----------------------------------------------
        case MIGV_TIMER_TIMESTAMP_IRQ:
        {
            migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMESTAMP]);
            migv_hw_timer_handle_irq(timer);
            // timer->high_bits++;
            return 0;
        }

        //----------------------------------------------
        default:
            break;
    }

    ZF_LOGE("unknown irq %d", irq->irq.number);
    return EINVAL;
}


//------------------------------------------------------------------------------
int set_timeout(
    void *data,
    uint64_t ns,
    timeout_type_t type)
{
    if (type == TIMEOUT_ABSOLUTE)
    {
        uint64_t time;
        int error = get_time(data, &time);
        if (error)
        {
            return error;
        }

        if (time > ns)
        {
            return ETIME;
        }

        ns -= time;
    }

    migv_timer_ctx_t *timer_ctx = data;
    migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMEOUT]);

    return migv_hw_timer_set_timeout(timer, ns, (type == TIMEOUT_PERIODIC));
}


//------------------------------------------------------------------------------
static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}


//------------------------------------------------------------------------------
static int reset(void *data)
{
    migv_timer_ctx_t *timer_ctx = data;

    // The seven timer driver tests are passing even if the stop and start
    // below is disabled. From the assumed interface semantics it seems natural
    // to stop and start the timer.

    migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMEOUT]);

    // restart the RTC
    migv_hw_timer_stop(timer);
    migv_hw_timer_start(timer);

    return 0;
}


//------------------------------------------------------------------------------
static void destroy(void *data)
{
    migv_timer_ctx_t *timer_ctx = data;
    migv_hw_timer_t *timer;

    timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMEOUT]);
    if (NULL != timer->regs)
    {
        migv_hw_timer_stop(timer);
        pmem_region_t region = {
            .length = PAGE_SIZE_4K,
            .base_addr = (uintptr_t)MIGV_TIMER_TIMEOUT_PADDR,
        };
        ps_pmem_unmap(&timer_ctx->ops, region, (void *)(timer->regs));
    }

    timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMESTAMP]);
    if (NULL != timer->regs)
    {
        migv_hw_timer_stop(timer);
        pmem_region_t region = {
            .length = PAGE_SIZE_4K,
            .base_addr = (uintptr_t)MIGV_TIMER_TIMESTAMP_PADDR,
        };
        ps_pmem_unmap(&timer_ctx->ops, region, (void *)(timer->regs));
    }
}


//------------------------------------------------------------------------------
int ltimer_default_describe(
    ltimer_t *ltimer,
    ps_io_ops_t ops)
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


//------------------------------------------------------------------------------
int ltimer_default_init(
    ltimer_t *ltimer,
    ps_io_ops_t ops,
    ltimer_callback_fn_t callback,
    void *callback_token)
{
    assert(ltimer != NULL);

    static migv_timer_ctx_t migv_timer_ctx = {0};

    migv_timer_ctx_t *timer_ctx = &migv_timer_ctx;
    timer_ctx->ops = ops;

    ltimer->data = timer_ctx;

    // setup callbacks
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int ret = ltimer_default_describe(ltimer, ops);
    assert(0 != ret); // this cannot fail

    // setup MIGV_TIMER_TIMESTAMP
    {
        migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMESTAMP]);

        timer->callback.func = callback;
        timer->callback.token = callback_token;
        timer->callback.data.ltimer = ltimer;
        timer->callback.data.irq_handler = handle_irq;
        timer->callback.data.irq = &(timer->ps_irq);

        pmem_region_t region = {
            .length = PAGE_SIZE_4K,
            .base_addr = (uintptr_t)MIGV_TIMER_TIMESTAMP_PADDR,
        };

        timer->regs = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
        if (timer->regs == NULL)
        {
            ZF_LOGE("ps_pmem_map() for MIGV_TIMER_TIMESTAMP failed");
            destroy(timer_ctx);
            return ENOMEM;
        }
    }

    // setup MIGV_TIMER_TIMEOUT
    {
        migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMEOUT]);

        timer->callback.func = callback;
        timer->callback.token = callback_token;
        timer->callback.data.ltimer = ltimer;
        timer->callback.data.irq_handler = handle_irq;
        timer->callback.data.irq = &(timer->ps_irq);


        pmem_region_t region = {
            .length = PAGE_SIZE_4K,
            .base_addr = (uintptr_t)MIGV_TIMER_TIMEOUT_PADDR,
        };

        timer->regs = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
        if (NULL == timer->regs)
        {
            ZF_LOGE("ps_pmem_map() for MIGV_TIMER_TIMEOUT failed");
            destroy(timer_ctx);
            return ENOMEM;
        }

        timer->ps_irq.type = PS_INTERRUPT;
        timer->ps_irq.irq.number = MIGV_TIMER_TIMEOUT_IRQ;


        timer->irq_id = ps_irq_register(
                            &ops.irq_ops,
                            timer->ps_irq,
                            handle_irq_wrapper,
                            &timer->callback.data);
        if (timer->irq_id < 0) {
            destroy(timer_ctx);
            return EIO;
        }
    }


    // set up timer for timestamp
    {
        migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMESTAMP]);
        migv_hw_timer_reset(timer);
        migv_set_timeout_ticks(timer, UINT32_MAX, true);
        migv_hw_timer_start(timer);
    }

    // set up timer for timeouts
    {
        migv_hw_timer_t *timer = &(timer_ctx->hw_timers[MIGV_TIMER_TIMEOUT]);
        migv_hw_timer_reset(timer);
        migv_hw_timer_start(timer);
    }

    return ret;
}

