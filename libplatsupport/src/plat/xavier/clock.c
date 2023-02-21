/*
 * Copyright (C) 2022, HENSOLDT Cyber GmbH
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <platsupport/clock.h>

/*
 * This is here just to make the compiler happy. The actual clock driver is
 * located inside libplatsupportports in the projects_libs repository.
 */
clk_t *ps_clocks = NULL;
freq_t ps_freq_default = {0};
