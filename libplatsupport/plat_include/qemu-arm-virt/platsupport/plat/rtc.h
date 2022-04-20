/*
 * Copyright (C) 2022, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <platsupport/timer.h>

#define RTC_PADDR 0x9010000

/*
 * from virt.dts: interrupts = < 0x00 0x02 0x04 >;
 * 0x00 -> IRQ_TYPE_SPI
 * 0x02 -> RTC IRQ = 32 + 2 = 34
 * 0x04 -> type of interrupt = Level sensitive, active high
 */

#define RTC_IRQ 34

typedef struct {
    void *vaddr;
} rtc_config_t;

typedef struct {
    uint32_t rtcdr;     /* Data Register (R): returns current value of 1Hz upcounter */
    uint32_t rtcmr;     /* Match Register (R/W): required for the alarm (IRQ) feature.
                         * Generates an IRQ when rtcmr == rtcdr. */
    uint32_t rtclr;     /* Load Register (R/W): set the initial value of the upcounter */
    uint32_t rtccr;     /* Control Register (R/W): writing 1 enables RTC, writing 0 disables it */
    uint32_t rtcimsc;   /* Interrupt Mask Set or Clear Register (R/W): writing 1 enables the interrupt, writing 0 disables it. */
    uint32_t rtcris;    /* Raw Interrupt Status (R): current raw status value of the corresponding interrupt, prior to masking*/
    uint32_t rtcmis;    /* Masked Interrupt Status (R): gives the current masked status value of the corresponding interrupt*/
    uint32_t rtcicr;    /* Interrupt Clear Register (W): writing 1 clears the interrupt, writing 0 has no effect.*/
} rtc_regs_t;

typedef struct {
    volatile rtc_regs_t *rtc_regs;
} rtc_t;

int rtc_start(rtc_t *rtc);
int rtc_stop(rtc_t *rtc);
void rtc_handle_irq(rtc_t *rtc, uint32_t irq);
uint64_t rtc_get_time(rtc_t *rtc);
int rtc_set_timeout(rtc_t *rtc, uint32_t ns, bool periodic);
int rtc_init(rtc_t *rtc, rtc_config_t config);