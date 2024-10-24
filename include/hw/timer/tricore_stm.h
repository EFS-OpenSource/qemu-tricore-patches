/*
 * QEMU model of the TriCore STM device.
 *
 * Copyright (c) 2017 David Brenken
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef HW_TRICORE_STM_H
#define HW_TRICORE_STM_H

#include "qemu/osdep.h"
#include "qemu/main-loop.h"
#include "hw/sysbus.h"
#include "hw/hw.h"
#include "hw/ptimer.h"
#include "hw/tricore/tricore_scu.h"


#define TYPE_TRICORE_STM "tricore_stm"
#define TRICORE_STM(obj) \
   OBJECT_CHECK(TriCoreSTMState, (obj), TYPE_TRICORE_STM)
#define NUM_CORES 1

#define MASK_ICR_CMP0EN 0x01
#define MASK_ICR_CMP0IR 0x02
#define MASK_ICR_CMP1EN 0x10
#define MASK_ICR_CMP1IR 0x20
#define MASK_ISCR_CMP0IRR 0x1
#define MASK_ISCR_CMP0IRS 0x2
#define MASK_ISCR_CMP1IRR 0x4
#define MASK_ISCR_CMP1IRS 0x8
#define MASK_CMCON_MSIZE0 0x1F
#define MASK_CMCON_MSTART0 0x1F00
#define MASK_CMCON_MSIZE1 0x1F0000
#define MASK_CMCON_MSTART1 0x1F000000

#define STM_R_MAX (0x100/4)

#define MASK_STM_CLC_DISS 0x2
#define MASK_SRC_STM0SR0_SRE 0x400


/* reset values */
#define RESET_TRICORE_STM_CLC 0x0
#define RESET_TRICORE_STM_ID 0x0000C000
#define RESET_TRICORE_STM_TIM0 0x0
#define RESET_TRICORE_STM_TIM1 0x0
#define RESET_TRICORE_STM_TIM2 0x0
#define RESET_TRICORE_STM_TIM3 0x0
#define RESET_TRICORE_STM_TIM4 0x0
#define RESET_TRICORE_STM_TIM5 0x0
#define RESET_TRICORE_STM_TIM6 0x0
#define RESET_TRICORE_STM_CAP 0x0
#define RESET_TRICORE_STM_CMP0 0x0
#define RESET_TRICORE_STM_CMP1 0x0
#define RESET_TRICORE_STM_CMCON 0x0
#define RESET_TRICORE_STM_ICR 0x0
#define RESET_TRICORE_STM_ISCR 0x0
#define RESET_TRICORE_STM_TIM0SV 0x0
#define RESET_TRICORE_STM_CAPSV 0x0
#define RESET_TRICORE_STM_OCS 0x0
#define RESET_TRICORE_STM_KRSTCLR 0x0
#define RESET_TRICORE_STM_KRST1 0x0
#define RESET_TRICORE_STM_KRST0 0x0
#define RESET_TRICORE_STM_ACCEN1 0x0
#define RESET_TRICORE_STM_ACCEN0 0xFFFFFFFF
#define RESET_TRICORE_STM_FREQUENCY 100000000



typedef struct {
    /* <private> */
    SysBusDevice parent_obj;
    char cmp0_irq_pending;
    QEMUBH *bh;                /* workaround TODO move to dedicated device */
    ptimer_state *ptimer;    /* workaround TODO move to dedicated device */

    /* <public> */
    MemoryRegion iomem;
    uint32_t regs[STM_R_MAX];
    MemoryRegion srvcreqregs; /* workaround */
    uint32_t SRC_STM0SR0;    /* workaround TODO move to dedicated device */
    uint32_t SRC_STM0SR1;    /* workaround TODO move to dedicated device */
    uint32_t SRC_STM1SR0;    /* workaround TODO move to dedicated device */
    uint32_t SRC_STM1SR1;    /* workaround TODO move to dedicated device */
    uint32_t SRC_STM2SR0;    /* workaround TODO move to dedicated device */
    uint32_t SRC_STM2SR1;    /* workaround TODO move to dedicated device */
    TriCoreSCUState *scu;
    qemu_irq irq;
    uint32_t freq_hz;
    uint64_t tim_counter;
} TriCoreSTMState;

#endif

