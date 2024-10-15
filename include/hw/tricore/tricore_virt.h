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


#ifndef HW_TRICORE_VIRT_H
#define HW_TRICORE_VIRT_H

#include "hw/sysbus.h"
#include "hw/hw.h"

#define TYPE_TRICORE_VIRT "tricore_virt"
#define TRICORE_VIRT(obj) \
   OBJECT_CHECK(TriCoreVIRTState, (obj), TYPE_TRICORE_VIRT)
#define NUM_CORES 1

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion iomem;

} TriCoreVIRTState;

#endif

