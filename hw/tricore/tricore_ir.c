/*
 * QEMU TriCore Interrupt Router (IR).
 *
 * Copyright (c) 2017 David Brenken <david.brenken@efs-auto.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "qapi/error.h"
#include "cpu.h"
#include "hw/irq.h"
#include "qemu/log.h"
#include "qemu/config-file.h"
#include "qemu/error-report.h"
#include "hw/tricore/tricore_ir.h"

static void tricore_ir_cpu_handler(void *opaque, int irq, int level)
{
    TriCoreCPU *cpu = opaque;
    CPUState *cs = CPU(cpu);

    if (level) {
        cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    } else {
        cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
    }
}

void tricore_check_interrupts(CPUTriCoreState *env)
{
    TriCoreCPU *cpu = TRICORE_CPU(qemu_get_cpu(0));
    CPUState *cs = CPU(cpu);
    if (env->irq_pending) {
        env->irq_pending = 0;
        cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    }
}

qemu_irq *tricore_cpu_ir_init(TriCoreCPU *cpu)
{
    return qemu_allocate_irqs(tricore_ir_cpu_handler, cpu, 2);
}
