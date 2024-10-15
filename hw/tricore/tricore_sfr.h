/*
 * QEMU TriCore virtualization helper device.
 *
 * Copyright (c) 2017 David Brenken <david.brenken@efs-auto.de>
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */


#ifndef HW_TRICORE_SFR_H
#define HW_TRICORE_SFR_H

#include "hw/sysbus.h"
#include "hw/hw.h"

#define TYPE_TRICORE_SFR "tricore_sfr"
#define TRICORE_SFR(obj) \
   OBJECT_CHECK(TriCoreSFRState, (obj), TYPE_TRICORE_SFR)
#define NUM_CORES 1

#define TRICORE_SFR_SIZE 0x00400000

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion iomem;
    uint32_t regs[TRICORE_SFR_SIZE/sizeof(uint32_t)];

} TriCoreSFRState;

#endif

