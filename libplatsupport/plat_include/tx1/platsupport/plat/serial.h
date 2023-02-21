/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once
#include <platsupport/mach/serial.h>
#include <platsupport/mach/serial_tk1_tx1_defs.h>

#if defined(CONFIG_PLAT_JETSON_NANO_2GB_DEV_KIT)
#define PS_SERIAL_DEFAULT   NV_UARTB
#define DEFAULT_SERIAL_PADDR UARTB_PADDR
#define DEFAULT_SERIAL_INTERRUPT UARTB_IRQ
#else
#define PS_SERIAL_DEFAULT   NV_UARTA
#define DEFAULT_SERIAL_PADDR UARTA_PADDR
#define DEFAULT_SERIAL_INTERRUPT UARTA_IRQ
#endif
