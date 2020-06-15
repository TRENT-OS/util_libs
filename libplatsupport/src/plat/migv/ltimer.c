

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
    uint32_t currentTimer;
    uint32_t timerControl;
    uint32_t timerCompare;
} migv_timer_regs_t;

#define TIMER_ENABLE   BIT(0)



/* Timers */
typedef enum  {
    HCSC1_TIMER_0,
    HCSC1_TIMER_1,

    HCSC1_TIMER_TIMEOUT   = HCSC1_TIMER_0,
    HCSC1_TIMER_TIMESTAMP = HCSC1_TIMER_1,

    NUM_HCSC1_TIMERS = 2,
} hcsc1_id_t;


#define HCSC1_TIMER_0_PADDR         MIGV_TIMER_0_PADDR
#define HCSC1_TIMER_0_IRQ_TIMEOUT   MIGV_TIMER_0_IRQ_TIMEOUT

#define HCSC1_TIMER_1_PADDR         MIGV_TIMER_1_PADDR
#define HCSC1_TIMER_1_IRQ_OVERFLOW  MIGV_TIMER_1_IRQ_OVERFLOW


static const uintptr_t hcsc1_timer_paddrs[] = {
    [HCSC1_TIMER_0] = HCSC1_TIMER_0_PADDR,
    [HCSC1_TIMER_1] = HCSC1_TIMER_1_PADDR,
};

static const long hcsc1_timer_irqs[] = {
    [HCSC1_TIMER_0] = HCSC1_TIMER_0_IRQ_TIMEOUT,
    [HCSC1_TIMER_1] = HCSC1_TIMER_1_IRQ_OVERFLOW,
};


// static pmem_region_t migv_timer_pmems[] = {
//     {
//         .type      = PMEM_TYPE_DEVICE,
//         .base_addr = HCSC1_TIMER_0_PADDR,
//         .length    = PAGE_SIZE_4K
//     },
//     {
//         .type      = PMEM_TYPE_DEVICE,
//         .base_addr = HCSC1_TIMER_1_PADDR,
//         .length    = PAGE_SIZE_4K
//     }
// };
//
//
// static ps_irq_t migv_timer_irqs[] = {
//     {
//         .type = PS_INTERRUPT,
//         .irq.number = HCSC1_TIMER_0_IRQ_TIMEOUT
//     },
//     {
//         .type = PS_INTERRUPT,
//         .irq.number = HCSC1_TIMER_1_IRQ_OVERFLOW
//     }
// };



typedef struct {
    migv_timer_regs_t *regs;
    bool singleShotTimeoutIsActive;
} hcsc1_t;


typedef struct {
    ps_io_ops_t ops;
    timer_callback_data_t callback_data;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    hcsc1_t hcsc1s[NUM_HCSC1_TIMERS];
    irq_id_t irq_id;
} migv_timer_t;


static UNUSED timer_properties_t hcsc1_timer_props = {
    .upcounter = false,
    .timeouts = true,
    .absolute_timeouts = false,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .bit_width = 32,
    .irqs = 1
};


//-----------------------------------------------------------------------------
static uint64_t hcsc1_ticks_to_ns(
    uint64_t ticks) {

    return ticks / TICKS_PER_MS * NS_IN_MS;
}


//-----------------------------------------------------------------------------
static void *hcsc1_get_paddr(hcsc1_id_t id)
{
    if (id < NUM_HCSC1_TIMERS && id >= HCSC1_TIMER_0) {
        return (void *) hcsc1_timer_paddrs[id];
    }

    return NULL;
}


//-----------------------------------------------------------------------------
static long hcsc1_get_irq(
    hcsc1_id_t id)
{
    if ((id < NUM_HCSC1_TIMERS) && (id >= HCSC1_TIMER_0)) {
        return hcsc1_timer_irqs[id];
    }

    return 0;
}


//-----------------------------------------------------------------------------
static void hcsc1_timer_reset(
    hcsc1_t *hcsc1)
{
    assert(NULL != hcsc1);

    /* Disable; pre-scaler of zero. */
    hcsc1->regs->timerControl = 0;

    hcsc1->singleShotTimeoutIsActive = false;
}


//-----------------------------------------------------------------------------
static void hcsc1_stop(
    hcsc1_t *hcsc1)
{
    assert(NULL != hcsc1);

    // clear enable flag
    hcsc1->regs->timerControl &= ~TIMER_ENABLE;
}


//-----------------------------------------------------------------------------
static void hcsc1_start(
    hcsc1_t *hcsc1)
{
    assert(NULL != hcsc1);

    // set enable flag
    hcsc1->regs->timerControl |= TIMER_ENABLE;
}


//-----------------------------------------------------------------------------
static uint64_t hcsc1_get_ticks(
    hcsc1_t *hcsc1)
{
    assert(NULL != hcsc1);

    return hcsc1->regs->currentTimer;
}


//-----------------------------------------------------------------------------
static uint64_t hcsc1_get_time(
    hcsc1_t *hcsc1)
{
    assert(NULL != hcsc1);

    uint64_t ticks = hcsc1_get_ticks(hcsc1);

    return hcsc1_ticks_to_ns(ticks);
}


//-----------------------------------------------------------------------------
static void hcsc1_set_timeout_ticks(
    hcsc1_t *hcsc1,
    uint32_t ticks,
    bool periodic)
{
    assert(NULL != hcsc1);

    hcsc1_stop(hcsc1);

    if (!periodic)
    {
        hcsc1->singleShotTimeoutIsActive = true;
    }

    // Set the timer value
    hcsc1->regs->timerCompare = ticks;

    // Enable the timer interrupt
    hcsc1_start(hcsc1);
}


//-----------------------------------------------------------------------------
static int hcsc1_set_timeout(
    hcsc1_t *hcsc1,
    uint64_t ns,
    bool periodic)
{
    assert(NULL != hcsc1);

    uint64_t ticks64 = ns * TICKS_PER_MS / NS_IN_MS;

    if (ticks64 >= UINT32_MAX) {
        ZF_LOGE("timeout value too large.");
        return ETIME;
    }

    hcsc1_set_timeout_ticks(hcsc1, ticks64, periodic);

    return 0;
}


//-----------------------------------------------------------------------------
static bool hcsc1_is_irq_pending(
    hcsc1_t *hcsc1) {

    assert(NULL != hcsc1);

    // ToDo ....
    return false;
}

//-----------------------------------------------------------------------------
static  void hcsc1_handle_irq(
    hcsc1_t *hcsc1)
{
    assert(NULL != hcsc1);

    if (hcsc1->singleShotTimeoutIsActive)
    {

        // if (timer->user_callback) {
        //     timer->user_callback(
        //      timer->user_callback_token,
        //      LTIMER_TIMEOUT_EVENT);
        // }



        hcsc1->singleShotTimeoutIsActive = false;
        hcsc1_stop(hcsc1);
    }
}




//-----------------------------------------------------------------------------
// ltimer callback
static size_t get_num_irqs(
    void *data)
{
    assert(NULL != data);

    return 2;
}


//-----------------------------------------------------------------------------
// ltimer callback
static int get_nth_irq(
    void *data,
    size_t n,
    ps_irq_t *irq)
{
    assert(NULL != data);
    assert(NULL != irq);

    assert(n < get_num_irqs(data));

    irq->type = PS_INTERRUPT;
    irq->irq.number = hcsc1_get_irq(n);

    return 0;
}


//-----------------------------------------------------------------------------
// ltimer callback
static size_t get_num_pmems(
    void *data)
{
    assert(NULL != data);

    return 2;
}


//-----------------------------------------------------------------------------
// ltimer callback
static int get_nth_pmem(
    void *data,
    size_t n,
    pmem_region_t *region)
{
    assert(NULL != data);
    assert(NULL != region);

    assert(n < get_num_pmems(data));

    region->length = PAGE_SIZE_4K;
    region->base_addr = (uintptr_t)hcsc1_get_paddr(n);

    return 0;
}

//-----------------------------------------------------------------------------
static int get_time(
    void *data,
    uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    migv_timer_t *timer = data;

    hcsc1_t *timer_timestamp = &(timer->hcsc1s[HCSC1_TIMER_TIMESTAMP]);
    uint64_t low_ticks = hcsc1_get_ticks(timer_timestamp);

    // uint64_t ticks = timer->high_bits + !!hcsc1_is_irq_pending(timer_timestamp);
    // ticks = (ticks << 32llu) + low_ticks;

    // *time = hcsc1_ticks_to_ns(ticks);

    return 0;
}

//-----------------------------------------------------------------------------
int handle_irq(
    void *data,
    ps_irq_t *irq)
{
    assert(data != NULL);

    migv_timer_t *timer = data;

    if (irq->irq.number == hcsc1_get_irq(HCSC1_TIMER_TIMEOUT)) {

        hcsc1_t *timer_timeout = &(timer->hcsc1s[HCSC1_TIMER_TIMEOUT]);
        hcsc1_handle_irq(timer_timeout);
        return 0;
    }

    if (irq->irq.number == hcsc1_get_irq(HCSC1_TIMER_TIMESTAMP)) {

        hcsc1_t *timer_timestamp = &(timer->hcsc1s[HCSC1_TIMER_TIMESTAMP]);
        hcsc1_handle_irq(timer_timestamp);
        // timer->high_bits++;
        return 0;
    }

    ZF_LOGE("unknown irq %d", irq->irq.number);
    return EINVAL;
}


//-----------------------------------------------------------------------------
int set_timeout(
    void *data,
    uint64_t ns,
    timeout_type_t type)
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

    migv_timer_t *timer = data;
    hcsc1_t *timer_timeout = &(timer->hcsc1s[HCSC1_TIMER_TIMEOUT]);

    return hcsc1_set_timeout(timer_timeout, ns, type == TIMEOUT_PERIODIC);
}


//-----------------------------------------------------------------------------
static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}


//-----------------------------------------------------------------------------
static int reset(void *data)
{
    migv_timer_t *timer = data;

    // The seven timer driver tests are passing even if the stop and start below is disabled.
    // From the assumed interface semantics it seems natural to stop and start the timer.

    hcsc1_t *timer_timeout = &(timer->hcsc1s[HCSC1_TIMER_TIMEOUT]);

    /* restart the rtc */
    hcsc1_stop(timer_timeout);
    hcsc1_start(timer_timeout);
    return 0;
}


//-----------------------------------------------------------------------------
static void destroy(void *data)
{
    pmem_region_t region;
    UNUSED int error;
    migv_timer_t *timer = data;

    hcsc1_t *timer_timeout = &(timer->hcsc1s[HCSC1_TIMER_TIMEOUT]);
    if (NULL != timer_timeout->regs) {
        hcsc1_stop(timer_timeout);
        error = get_nth_pmem(data, 0,  &region);
        assert(!error);
        ps_pmem_unmap(&timer->ops, region, (void *)(timer_timeout->regs));
    }

    hcsc1_t *timer_timestamp = &(timer->hcsc1s[HCSC1_TIMER_TIMESTAMP]);
    if (NULL != timer_timestamp->regs) {
        hcsc1_stop(timer_timestamp);
        error = get_nth_pmem(data, 1,  &region);
        assert(!error);
        ps_pmem_unmap(&timer->ops, region, (void *)(timer_timestamp->regs));
    }

    ps_free(&timer->ops.malloc_ops, sizeof(timer), timer);
}


//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
int ltimer_default_init(
    ltimer_t *ltimer,
    ps_io_ops_t ops,
    ltimer_callback_fn_t callback,
    void *callback_token)
{
    assert(ltimer != NULL);

    int ret;

    ret = ltimer_default_describe(ltimer, ops);
    if (0 != ret) {
        ZF_LOGE("ltimer_default_describe() failed, code %d", ret);
        return ret;
    }

    // setup callbacks
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;


    /* allocate hardware timer context */
    ret = ps_calloc(&ops.malloc_ops, 1, sizeof(migv_timer_t), &ltimer->data);
    if (0 != ret) {
        ZF_LOGE("ps_calloc() failed for malloc, code %d", ret);
        return ret;
    }

    assert(ltimer->data != NULL);
    migv_timer_t *timer = (migv_timer_t *)(ltimer->data);

    /* setup hardware timer context */
    timer->ops = ops;
    timer->irq_id = 0; // PS_INVALID_IRQ_ID;
    timer->user_callback = callback;
    timer->user_callback_token = callback_token;

    /* map timer peripherals */
    pmem_region_t region = {0};

    hcsc1_t *timer_timeout = &(timer->hcsc1s[HCSC1_TIMER_TIMEOUT]);
    ret = get_nth_pmem(NULL, 0, &region);
    assert(0 == ret);
    timer_timeout->regs = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (NULL == timer_timeout->regs) {
        ZF_LOGE("ps_pmem_map() region 0 failed");
        destroy(timer);
        return ENOMEM;
    }


    hcsc1_t *timer_timestamp = &timer->hcsc1s[HCSC1_TIMER_TIMESTAMP];
    ret = get_nth_pmem(NULL, 1, &region);
    assert(0 == ret);
    timer_timestamp->regs = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (timer_timestamp->regs == NULL) {
        ZF_LOGE("ps_pmem_map() region 1 failed");
        destroy(timer);
        return ENOMEM;
    }


//    /* register the IRQs we need */
//    timer->callback_data.ltimer = ltimer;
//    timer->callback_data.irq_handler = handle_irq;
//
//    ret = ps_calloc(
//            &(ops.malloc_ops),
//            1,
//            sizeof(ps_irq_t),
//            (void **)(&(timer->callback_data.irq)));
//    if (0 != ret) {
//        ZF_LOGE("ps_calloc() failed for malloc, code %d", ret);
//        destroy(timer);
//        return ret;
//    }
//
//    error = get_nth_irq(ltimer->data, 0, timer->callback_data.irq);
//    if (error) {
//        destroy(ltimer->data);
//        return error;
//    }
//
//    timer->irq_id = ps_irq_register(
//                        &ops.irq_ops,
//                        *timer->callback_data.irq,
//                        handle_irq_wrapper,
//                        &timer->callback_data);
//    if (timer->irq_id < 0) {
//        destroy(ltimer->data);
//        return EIO;
//    }


    /* set up timer for timestamp */
    hcsc1_timer_reset(timer_timestamp);
    hcsc1_set_timeout_ticks(timer_timestamp, UINT32_MAX, true);
    hcsc1_start(timer_timestamp);

    /* set up timer for timeouts */
    hcsc1_timer_reset(timer_timeout);
    hcsc1_start(timer_timeout);


    return ret;
}



//
// // WARNING: code from ariane, does not work on migv
//
// #include <stdio.h>
// #include <assert.h>
// #include <errno.h>
// #include <stdlib.h>
// #include <stdint.h>
// #include <stdbool.h>
// #include <platsupport/timer.h>
// #include <platsupport/ltimer.h>
// #include <platsupport/pmem.h>
// #include <utils/util.h>
// #include "../../ltimer.h"
//
// // the ltimer is build on the APB timer
//
// #define APB_TIMER_PADDR         0x18000000
//
// /* The input frequence is the CPU frequency which is 50MHz by default */
// #define APB_TIMER_INPUT_FREQ    (50*1000*1000)
// #define APB_TIMER_SCALING       (NS_IN_S / APB_TIMER_INPUT_FREQ)
//
// /* Largest timeout that can be programmed */
// #define MAX_TIMEOUT_NS          (BIT(31) * APB_TIMER_SCALING)
//
//
// /* Multiple timers */
// #define APB_TIMER_DIST          0x10
// #define APB_TIMER_NUM           2
// #define APB_TIMER_BASE(n)       (APB_TIMER_DIST * (n))
//
// #define CMP_WIDTH               32
// #define APB_TIMER_CTRL_ENABLE   BIT(0);
// #define CMP_MASK                MASK(CMP_WIDTH)
//
// /* Timer IRQs */
// #define APB_TIMER_PLIC_BASE     4
// #define APB_TIMER_IRQ_BASE(n)   (APB_TIMER_PLIC_BASE + 2*(n))
// #define APB_TIMER_IRQ_OVF(n)    APB_TIMER_IRQ_BASE(n)
// #define APB_TIMER_IRQ_CMP(n)    ( APB_TIMER_IRQ_BASE(n) + 1)
//
//
// typedef struct {
//     uint32_t time;
//     uint32_t ctrl;
//     uint32_t cmp;
// } apb_timer_map_t;
//
//
// typedef struct {
//     volatile apb_timer_map_t *apb_timer_map;
//     uint64_t time_h;
// } apb_timer_t;
//
//
// typedef struct {
//     apb_timer_t apb_timer;
//     void *vaddr;
// } apb_timer_ltimer_t;
//
//
// typedef struct {
//     apb_timer_ltimer_t apb_timer_ltimer;
//     ps_io_ops_t ops;
//     irq_id_t irq_id;
//     ltimer_callback_fn_t user_callback;
//     timer_callback_data_t callback_data;
//     void *user_callback_token;
//     uint64_t period;
// } ariane_ltimer_t;
//
//
// static pmem_region_t pmems[] = {
//     {
//         .type = PMEM_TYPE_DEVICE,
//         .base_addr = APB_TIMER_PADDR,
//         .length = PAGE_SIZE_4K
//     }
// };
//
//
// static ps_irq_t irqs[] = {
//     {
//         .type = PS_INTERRUPT,
//         .irq.number = APB_TIMER_IRQ_CMP(0)
//
//     }
// };
//
//
// //------------------------------------------------------------------------------
// static void apb_timer_start(
//     apb_timer_t *apb_timer)
// {
//     apb_timer->apb_timer_map->ctrl |= APB_TIMER_CTRL_ENABLE;
// }
//
//
// //------------------------------------------------------------------------------
// static void apb_timer_stop(
//     apb_timer_t *apb_timer)
// {
//     /* Disable timer. */
//     apb_timer->apb_timer_map->cmp = CMP_MASK;
//     apb_timer->apb_timer_map->ctrl &= ~APB_TIMER_CTRL_ENABLE;
//     apb_timer->apb_timer_map->time = 0;
// }
//
//
// //------------------------------------------------------------------------------
// static int apb_timer_set_timeout(
//     apb_timer_t *apb_timer,
//     uint64_t ns)
// {
//     if (ns > MAX_TIMEOUT_NS) {
//         ZF_LOGE("Cannot program a timeout larget than %ld ns", MAX_TIMEOUT_NS);
//         return -1;
//     }
//
//     apb_timer->apb_timer_map->cmp = ns / APB_TIMER_SCALING;
//     apb_timer_start(apb_timer);
//
//     return 0;
// }
//
//
// //------------------------------------------------------------------------------
// static uint64_t apb_timer_get_time(
//     apb_timer_t *apb_timer)
// {
//     uint64_t ticks = apb_timer->apb_timer_map->time + apb_timer->time_h;
//     return ticks * APB_TIMER_SCALING;
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// size_t get_num_irqs(
//     void *data)
// {
//     return 1;
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static int get_nth_irq(
//     void *data,
//     size_t n,
//     ps_irq_t *irq)
// {
//     assert(data != NULL);
//     assert(irq != NULL);
//     assert(n == 0);
//
//     irq->irq.number = APB_TIMER_IRQ_CMP(0);
//     irq->type = PS_INTERRUPT;
//     return 0;
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static size_t get_num_pmems(
//     void *data)
// {
//     return sizeof(pmems) / sizeof(pmems[0]);
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static int get_nth_pmem(
//     void *data,
//     size_t n,
//     pmem_region_t *paddr)
// {
//     assert(n < get_num_pmems(data));
//
//     *paddr = pmems[n];
//
//     return 0;
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static int get_time(
//     void *data,
//     uint64_t *time)
// {
//     assert(data != NULL);
//     assert(time != NULL);
//
//     ariane_ltimer_t *timer = data;
//     apb_timer_t *apb_timer = &timer->apb_timer_ltimer.apb_timer;
//
//     *time = apb_timer_get_time(apb_timer);
//     return 0;
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static int get_resolution(
//     void *data,
//     uint64_t *resolution)
// {
//     return ENOSYS;
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static int set_timeout(
//     void *data,
//     uint64_t ns,
//     timeout_type_t type)
// {
//     assert(data != NULL);
//     ariane_ltimer_t *timer = data;
//     timer->period = 0;
//     apb_timer_t *apb_timer = &timer->apb_timer_ltimer.apb_timer;
//
//     switch (type) {
//         case TIMEOUT_ABSOLUTE: {
//             uint64_t time = apb_timer_get_time(apb_timer);
//             if (time >= ns) {
//                 return ETIME;
//             }
//             apb_timer_set_timeout(apb_timer, ns - time);
//             return 0;
//         }
//         case TIMEOUT_PERIODIC: {
//             timer->period = ns;
//             /* fall through */
//         }
//         case TIMEOUT_RELATIVE: {
//             apb_timer_set_timeout(apb_timer, ns);
//             return 0;
//         }
//     }
//
//     return EINVAL;
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static int handle_irq(
//     void *data,
//     ps_irq_t *irq)
// {
//     assert(data != NULL);
//
//     ariane_ltimer_t *timer = data;
//     apb_timer_t *apb_timer = &timer->apb_timer_ltimer.apb_timer;
//
//     apb_timer->time_h += apb_timer->apb_timer_map->cmp;
//     apb_timer_stop(apb_timer);
//
//     if (timer->period > 0) {
//         apb_timer_set_timeout(apb_timer, timer->period);
//     }
//
//     ltimer_event_t event = LTIMER_TIMEOUT_EVENT;
//     if (timer->user_callback) {
//         timer->user_callback(timer->user_callback_token, event);
//     }
//     apb_timer_start(apb_timer);
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static int reset(
//     void *data)
// {
//     assert(data != NULL);
//
//     ariane_ltimer_t *timer = data;
//     apb_timer_t *apb_timer = &timer->apb_timer_ltimer.apb_timer;
//
//     apb_timer_stop(apb_timer);
//     apb_timer_start(apb_timer);
//
//     return 0;
// }
//
//
// //------------------------------------------------------------------------------
// // ltimer callback
// static void destroy(
//     void *data)
// {
//     assert(data != NULL);
//
//     ariane_ltimer_t *timer = data;
//     apb_timer_t *apb_timer = &timer->apb_timer_ltimer.apb_timer;
//
//     if (timer->apb_timer_ltimer.vaddr) {
//         apb_timer_stop(apb_timer);
//         ps_pmem_unmap(&timer->ops, pmems[0], timer->apb_timer_ltimer.vaddr);
//     }
//
//     if (timer->irq_id > PS_INVALID_IRQ_ID) {
//         int error = ps_irq_unregister(&timer->ops.irq_ops, timer->irq_id);
//         ZF_LOGF_IF(error, "Failed to unregister IRQ");
//     }
//     ps_free(&timer->ops.malloc_ops, sizeof(timer), timer);
// }
//
//
// //------------------------------------------------------------------------------
// int ltimer_default_describe(
//     ltimer_t *ltimer,
//     ps_io_ops_t ops)
// {
//     if (ltimer == NULL) {
//         ZF_LOGE("Timer is NULL!");
//         return EINVAL;
//     }
//
//     ltimer->get_num_irqs = get_num_irqs;
//     ltimer->get_nth_irq = get_nth_irq;
//     ltimer->get_num_pmems = get_num_pmems;
//     ltimer->get_nth_pmem = get_nth_pmem;
//
//     return 0;
// }
//
//
// //------------------------------------------------------------------------------
// int ltimer_default_init(
//     ltimer_t *ltimer,
//     ps_io_ops_t ops,
//     ltimer_callback_fn_t callback,
//     void *callback_token)
// {
//     assert(ltimer != NULL);
//
//     int error = ltimer_default_describe(ltimer, ops);
//     if (error) {
//         return error;
//     }
//
//     ltimer->get_time = get_time;
//     ltimer->get_resolution = get_resolution;
//     ltimer->set_timeout = set_timeout;
//     ltimer->reset = reset;
//     ltimer->destroy = destroy;
//
//     error = ps_calloc(
//                 &ops.malloc_ops,
//                 1,
//                 sizeof(ariane_ltimer_t),
//                 &ltimer->data);
//     if (error) {
//         return error;
//     }
//
//     assert(ltimer->data != NULL);
//     ariane_ltimer_t *timer = ltimer->data;
//
//     timer->ops = ops;
//     timer->user_callback = callback;
//     timer->user_callback_token = callback_token;
//     timer->irq_id = 0;//PS_INVALID_IRQ_ID;
//
//     /* map the registers we need */
//     timer->apb_timer_ltimer.vaddr = ps_pmem_map(
//                                         &ops,
//                                         pmems[0],
//                                         false,
//                                         PS_MEM_NORMAL);
//     if (timer->apb_timer_ltimer.vaddr == NULL) {
//         destroy(ltimer->data);
//         return ENOMEM;
//     }
//
//     /* register the IRQs we need */
//     error = ps_calloc(
//                 &ops.malloc_ops,
//                 1,
//                 sizeof(ps_irq_t),
//                 (void **) &timer->callback_data.irq);
//     if (error) {
//         destroy(ltimer->data);
//         return error;
//     }
//
//     timer->callback_data.ltimer = ltimer;
//     timer->callback_data.irq_handler = handle_irq;
//
//     error = get_nth_irq(ltimer->data, 0, timer->callback_data.irq);
//     if (error) {
//         destroy(ltimer->data);
//         return error;
//     }
//
//     timer->irq_id = ps_irq_register(
//                         &ops.irq_ops,
//                         *timer->callback_data.irq,
//                         handle_irq_wrapper,
//                         &timer->callback_data);
//     if (timer->irq_id < 0) {
//         destroy(ltimer->data);
//         return EIO;
//     }
//
//     /* setup apb_timer */
//     apb_timer_t *apb_timer = &timer->apb_timer_ltimer.apb_timer;
//     apb_timer->apb_timer_map = (volatile apb_timer_map_t *)timer->apb_timer_ltimer.vaddr;
//     apb_timer->time_h = 0;
//
//     apb_timer_start(apb_timer);
//
//     return 0;
// }
