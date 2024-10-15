/*
 *  QEMU model of the TriCore STM device.
 *
 *  Copyright (c) 2017 David Brenken
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */
#include "qemu/osdep.h"
#include "hw/qdev-properties.h"
#include "hw/timer/tricore_stm.h"
#include <stdio.h>
#include <inttypes.h>
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "qapi/error.h"
#include "sysemu/sysemu.h"
#include "hw/ptimer.h"
#include "hw/irq.h"
#include <math.h>

enum {
    CLC,
    RESERVED1,
    ID,
    RESERVED2,
    TIM0,
    TIM1,
    TIM2,
    TIM3,
    TIM4,
    TIM5,
    TIM6,
    CAP,
    CMP0,
    CMP1,
    CMCON,
    ICR,
    ISCR,
    RESERVED3,
    TIM0SV = 0x50 / 4,
    CAPSV,
    RESERVED4,
    OCS = 0xE8 / 4,
    KRSTCLR,
    KRST1,
    KRST0,
    ACCEN1,
    ACCEN0
};

static uint64_t tricore_stm_get_tim_update_regs(TriCoreSTMState *s,
        int timshift, char bUpdateTIM);

static void tricore_stm_update_irqs(void *opaque)
{
    TriCoreSTMState *s = (TriCoreSTMState *) opaque;

    if ((s->regs[ICR] & MASK_ICR_CMP0IR) && (s->regs[ICR] & MASK_ICR_CMP0EN)) {
        qemu_irq_raise(s->irq);
    } else {
        qemu_irq_lower(s->irq);
    }
}

static void tricore_stm_update_freq(TriCoreSTMState *s)
{
    ptimer_transaction_begin(s->ptimer);
    uint32_t freq = tricore_scu_get_stmclock(s->scu);
    s->freq_hz = freq;
    ptimer_set_freq(s->ptimer, freq);
    ptimer_transaction_commit(s->ptimer);
}

static void tricore_stm_timer_start(TriCoreSTMState *s)
{
    ptimer_transaction_begin(s->ptimer);
    ptimer_stop(s->ptimer);
    ptimer_transaction_commit(s->ptimer);

    if (!(s->regs[ICR] & MASK_ICR_CMP0EN)) {
        /* Since timer is disabled, no need to start one.*/
        return;
    }

    int shiftsToRight = ((s->regs[CMCON] & MASK_CMCON_MSTART0) >> 8)
            + ((s->regs[CMCON] & MASK_CMCON_MSIZE0) >> 8);

    uint64_t tim = tricore_stm_get_tim_update_regs(s, shiftsToRight, 0);

    /* ToDo Consider length. */

    uint64_t delta_ticks_2 = s->regs[CMP0] - tim;
    uint64_t timeout_ticks = (delta_ticks_2 * pow(2, shiftsToRight));

    ptimer_transaction_begin(s->ptimer);
    ptimer_set_limit(s->ptimer, timeout_ticks, 1);
    ptimer_run(s->ptimer, 1);
    ptimer_transaction_commit(s->ptimer);
}



static void tricore_stm_write(void *opaque, hwaddr offset, uint64_t value,
        unsigned size)
{
    TriCoreSTMState *s = (TriCoreSTMState *) opaque;

    /* Since we can only write a byte, we have to calculate and shift
     some values. */
    hwaddr reg_addr = offset >> 2;
    value = value << ((offset & 0x3) * 0x8);

    if (size == 1) {
        uint32_t old_value = s->regs[reg_addr];
        uint32_t shifter = offset & 3;
        old_value &= ~(0xFF << (shifter * 8));
        value |= old_value;
    }
    if (size == 2) {
        uint32_t old_value = s->regs[reg_addr];
        uint32_t shifter = offset & 3;
        old_value &= ~(0xFFFF << (shifter * 8));
        value |= old_value;
    }

    switch (reg_addr) {
    case CLC:
    case ID:
    case TIM0:
    case TIM1:
    case TIM2:
    case TIM3:
    case TIM4:
    case TIM5:
    case TIM6:
    case CAP:
    case CMP0:
    case CMP1:
    case ICR:
    case TIM0SV:
    case CAPSV:
    case OCS:
    case KRSTCLR:
    case KRST1:
    case KRST0:
    case ACCEN1:
    case ACCEN0:
        s->regs[reg_addr] = value;
        break;
    case CMCON:
        s->regs[reg_addr] = value;

        /* Start timer if necessary. */
        tricore_stm_timer_start(s);
        break;
    case ISCR:
        /* when reset flags are set, clear flags */
        if (value & MASK_ISCR_CMP0IRR) {
            qatomic_and(&s->regs[ICR], ~MASK_ICR_CMP0IR);
        }
        if (value & MASK_ISCR_CMP0IRS) {
            qatomic_or(&s->regs[ICR], MASK_ICR_CMP0IR);
        }
        if (value & MASK_ISCR_CMP1IRR) {
            qatomic_and(&s->regs[ICR], ~MASK_ICR_CMP1IR);
        }
        if (value & MASK_ISCR_CMP1IRS) {
            qatomic_or(&s->regs[ICR], MASK_ICR_CMP1IR);
        }
        break;
    default:
        break;
    }

    tricore_stm_update_irqs(opaque);
}

static uint64_t tricore_stm_get_tim_update_regs(TriCoreSTMState *s,
        int timshift, char bUpdateTIM)
{
    uint64_t r = 0x0;
    uint64_t ticks = 0x0;

    /* Time in ns is converted to time in s. */
    ticks = (qemu_clock_get_ns(QEMU_CLOCK_HOST) / (1000000000 / s->freq_hz));

    r = (uint32_t) (ticks >> timshift);

    if (bUpdateTIM) {
        s->tim_counter = ticks;
        s->regs[CAP] = (uint32_t) (s->tim_counter >> 32);
    }

    return r;
}


static uint64_t tricore_stm_read(void *opaque, hwaddr offset, unsigned size)
{
    TriCoreSTMState *s = (TriCoreSTMState *) opaque;
    uint64_t r = 0x0;
    /* Since we can only write a byte, we have to calculate and
     shift some values. */
    hwaddr reg_addr = offset >> 2;

    /* ToDo Move this function call to a proper location. */
    tricore_stm_update_freq(s);

    switch (reg_addr) {
    case CLC:
    case ID:
    case TIM0:
        r = tricore_stm_get_tim_update_regs(s, 0, 1);
        break;
    case TIM1:
        r = tricore_stm_get_tim_update_regs(s, 4, 1);
        break;
    case TIM2:
        r = tricore_stm_get_tim_update_regs(s, 8, 1);
        break;
    case TIM3:
        r = tricore_stm_get_tim_update_regs(s, 12, 1);
        break;
    case TIM4:
        r = tricore_stm_get_tim_update_regs(s, 16, 1);
        break;
    case TIM5:
        r = tricore_stm_get_tim_update_regs(s, 20, 1);
        break;
    case TIM6:
        r = tricore_stm_get_tim_update_regs(s, 32, 1);
        break;
    case CAP:
        r = s->regs[CAP];
        break;
    case ISCR:
        r = 0;
        break;
    case CMP0:
    case CMP1:
    case CMCON:
    case ICR:
        /* ToDo */
        break;
    case TIM0SV:
        r = 0x0; /* ToDo */
        s->regs[CAP] = (uint32_t) (s->tim_counter >> 32);
        break;
    case CAPSV:
    case OCS:
    case KRSTCLR:
    case KRST1:
    case KRST0:
    case ACCEN1:
    case ACCEN0:
        r = s->regs[reg_addr];
        break;
    default:
        error_report("tricore_stm: read access to unknown register 0x%02X", (uint16_t)offset);
        r = 0x0;
        break;
    }

    return r;
}


static void tricore_stm_reset(DeviceState *dev)
{
    TriCoreSTMState *s = TRICORE_STM(dev);

    s->regs[CLC] = RESET_TRICORE_STM_CLC;
    s->regs[ID] = RESET_TRICORE_STM_ID;
    s->regs[TIM0] = RESET_TRICORE_STM_TIM0;
    s->regs[TIM1] = RESET_TRICORE_STM_TIM1;
    s->regs[TIM2] = RESET_TRICORE_STM_TIM2;
    s->regs[TIM3] = RESET_TRICORE_STM_TIM3;
    s->regs[TIM4] = RESET_TRICORE_STM_TIM4;
    s->regs[TIM5] = RESET_TRICORE_STM_TIM5;
    s->regs[TIM6] = RESET_TRICORE_STM_TIM6;
    s->regs[CAP] = RESET_TRICORE_STM_CAP;
    s->regs[CMP0] = RESET_TRICORE_STM_CMP0;
    s->regs[CMP1] = RESET_TRICORE_STM_CMP1;
    s->regs[CMCON] = RESET_TRICORE_STM_CMCON;
    s->regs[ICR] = RESET_TRICORE_STM_ICR;
    s->regs[ISCR] = RESET_TRICORE_STM_ISCR;
    s->regs[TIM0SV] = RESET_TRICORE_STM_TIM0SV;
    s->regs[CAPSV] = RESET_TRICORE_STM_CAPSV;
    s->regs[OCS] = RESET_TRICORE_STM_OCS;
    s->regs[KRSTCLR] = RESET_TRICORE_STM_KRSTCLR;
    s->regs[KRST1] = RESET_TRICORE_STM_KRST1;
    s->regs[KRST0] = RESET_TRICORE_STM_KRST0;
    s->regs[ACCEN1] = RESET_TRICORE_STM_ACCEN1;
    s->regs[ACCEN0] = RESET_TRICORE_STM_ACCEN0;

}

static const MemoryRegionOps tricore_stm_ops = {
        .read = tricore_stm_read, .write = tricore_stm_write, .valid = {
                .min_access_size = 4, .max_access_size = 4, }, .endianness =
                DEVICE_NATIVE_ENDIAN, };

static void tricore_stm_timer_hit(void *opaque)
{
    TriCoreSTMState *s = (TriCoreSTMState *) opaque;

    if (!(s->regs[ICR] & MASK_ICR_CMP0EN)) {
        return;
    }

    /* set compare interrupt flag */
    qatomic_or(&s->regs[ICR], MASK_ICR_CMP0IR);

    tricore_stm_update_irqs(opaque);
}

static void tricore_stm_realize(DeviceState *dev, Error **errp)
{
    TriCoreSTMState *s = TRICORE_STM(dev);
    SysBusDevice *sbd = SYS_BUS_DEVICE(dev);
    Error *err = NULL;
    
    s->scu = (TriCoreSCUState *)object_property_get_link(OBJECT(dev), "scu", &err);
    if (!s->scu) {
        error_setg(errp, "tricore_stm: scu link not found: %s",
                error_get_pretty(err));
        return;
    }

    s->ptimer = ptimer_init(tricore_stm_timer_hit, s, PTIMER_POLICY_LEGACY);
    tricore_stm_update_freq(s);

    ptimer_transaction_begin(s->ptimer);
    ptimer_set_limit(s->ptimer, 1, 0);
    ptimer_run(s->ptimer, 0);
    ptimer_transaction_commit(s->ptimer);

    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static void tricore_stm_init(Object *obj)
{
    TriCoreSTMState *s = TRICORE_STM(obj);
    /* map memory */
    memory_region_init_io(&s->iomem, OBJECT(s), &tricore_stm_ops, s,
            "tricore_stm", 0xFF);
    s->tim_counter = 0x0;
}

static Property tricore_stm_properties[] = { DEFINE_PROP_END_OF_LIST() };

static void tricore_stm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    device_class_set_props(dc, tricore_stm_properties);
    dc->reset = tricore_stm_reset;
    dc->realize = tricore_stm_realize;
}

static const TypeInfo tricore_stm_info = {
        .name = TYPE_TRICORE_STM, .parent = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(TriCoreSTMState), .instance_init =
                tricore_stm_init, .class_init = tricore_stm_class_init, };

static void tricore_stm_register_types(void)
{
    type_register_static(&tricore_stm_info);
}

type_init(tricore_stm_register_types)
