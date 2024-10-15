/*
 * QEMU TriCore SCU device.
 *
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef HW_TRICORE_IR_H
#define HW_TRICORE_IR_H

#include "hw/sysbus.h"
#include "hw/hw.h"

#define TYPE_TRICORE_IR "tricore_ir"
#define TRICORE_IR(obj) \
   OBJECT_CHECK(TriCoreIRState, (obj), TYPE_TRICORE_IR)


qemu_irq *tricore_cpu_ir_init(TriCoreCPU *cpu);

#endif

