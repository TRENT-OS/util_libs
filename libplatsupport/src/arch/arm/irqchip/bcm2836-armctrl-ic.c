/*
 * Copyright (C) 2024, HENSOLDT Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <stdbool.h>

#include <utils/util.h>

#include "../../../irqchip.h"

#define BCM2836_ARMCTRL_INT_CELL_COUNT 2

typedef enum {
    INT_TYPE_OFFSET,
    INT_OFFSET
} interrupt_cell_offset;

static int parse_bcm_2836_armctrl_interrupts(char *dtb_blob, int node_offset, int intr_controller_phandle,
                                     irq_walk_cb_fn_t callback, void *token)
{
    bool is_extended = false;
    int prop_len = 0;
    const void *interrupts_prop = get_interrupts_prop(dtb_blob, node_offset, &is_extended, &prop_len);
    assert(interrupts_prop != NULL);
    int total_cells = prop_len / sizeof(uint32_t);

    if(is_extended) {
        ZF_LOGE("This parser doesn't understand extended interrupts");
        return ENODEV;
    }

    int stride = BCM2836_ARMCTRL_INT_CELL_COUNT + (is_extended == true);
    int num_interrupts = total_cells / stride;

    assert(total_cells % stride == 0);

    for (int i = 0; i < num_interrupts; i++) {
        ps_irq_t curr_irq = {0};
        const void *curr = interrupts_prop + (i * stride * sizeof(uint32_t));
        curr_irq.type = PS_INTERRUPT;
        uint32_t irq_type = 0;
        uint32_t irq = 0;

        irq_type = READ_CELL(1, curr, INT_TYPE_OFFSET);
        irq = READ_CELL(1, curr, INT_OFFSET);

        curr_irq.irq.number = irq;
        int error = callback(curr_irq, i, num_interrupts, token);
        if(error) {
            return error;
        }
    }

    return 0;
}

char *bcm_2836_armctrl_compatible_list[] = {
    "brcm,bcm2836-armctrl-ic",
    "brcm,bcm2836-l1-intc",
    NULL
};
DEFINE_IRQCHIP_PARSER(bcm_2836_armctrl, bcm_2836_armctrl_compatible_list, parse_bcm_2836_armctrl_interrupts);