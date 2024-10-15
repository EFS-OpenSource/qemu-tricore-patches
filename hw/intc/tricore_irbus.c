/*
 * QEMU TriCore Interrupt Router Bus.
 *
 * Copyright (c) 2017 David Brenken <david.brenken@efs-auto.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "qapi/error.h"

#include "qemu/log.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "qemu/main-loop.h"
#include "cpu.h"
#include "qemu/error-report.h"
#include "hw/intc/tricore_irbus.h"

enum {
    RESERVED = 0, RESERVED2, ASCLINUARTRX, ASCLINUARTTX, ASCLINUARTERR, STM
};

static const char *get_name_by_src(int srcnum)
{
    switch (srcnum) {
    case 0x20: /* SRC_ASCLINmTX offset: 0x80 */
        return "SRC_ASCLINmTX";
    case 0x21: /* SRC_ASCLINmRX offset: 0x84 */
        return "SRC_ASCLINmRX";
    case 0x22: /* SRC_ASCLINmEX offset: 0x88 */
        return "SRC_ASCLINmEX";
    case 0x30: /* SRC_STMmSR0 0 offset: 0x490 */
        return "SRC_STMmSR0";
    case 0x31: /* SRC_STMmSR1 0 offset: 0x494 */
        return "SRC_STMmSR1";
    case 0x32: /* SRC_STMmSR0 1 offset: 0x498 */
        return "SRC_STMmSR0";
    case 0x33: /* SRC_STMmSR1 1 offset: 0x49C */
        return "SRC_STMmSR1";
    case 0x34: /* SRC_STMmSR0 2 offset: 0x4A0 */
        return "SRC_STMmSR0";
    case 0x35: /* SRC_STMmSR1 2 offset: 0x4A4 */
        return "SRC_STMmSR1";
    case 254:
        return "RESET";
    default:
        return "";
    }
}

static void irq_evaluate(void *opaque)
{
    TriCoreIRBUSState *pv = opaque;
    CPUTriCoreState *env = &((TriCoreCPU *) (pv->cpu))->env;

    for (uint32_t srcnum = 0; srcnum < IR_SRC_COUNT; srcnum++) {
        uint32_t src_reg = pv->src_control_reg[srcnum];

        /* has request set and interrupt enabled? */
        if ((src_reg & IR_SRC_SRR) && ((src_reg & IR_SRC_SRE) || (srcnum == IR_SRC_RESET))) {

            if (qemu_loglevel_mask(CPU_LOG_INT)) {
                qemu_log("tricore_irbus: SRC #%d (%s) (SRPN %d) triggered\n",
                srcnum, get_name_by_src(srcnum), (src_reg & IR_SRC_SRPN));
            }

            /* we should disable SRR bit here, but we dont get confirmation */

            /* write back modified register */
            pv->src_control_reg[srcnum] = src_reg;

            if (qemu_loglevel_mask(CPU_LOG_INT)) {
                qemu_log("tricore_irbus: SRC %s (SRPN %d) triggered\n",
                get_name_by_src(srcnum),
                (pv->src_control_reg[srcnum] & IR_SRC_SRPN));
            } 
            env->ICR = (env->ICR & (~MASK_ICR_PIPN)) |
                ((src_reg & IR_SRC_SRPN) << 16);

            qemu_irq_raise(pv->parent_irq);

            return;
        }
    }

    if (qemu_loglevel_mask(CPU_LOG_INT)) {
        qemu_log("tricore_irbus: lower irq line\n");
    }
    qemu_irq_lower(pv->parent_irq);
}

static void irq_handler(void *opaque, int srcnum, int level)
{
    TriCoreIRBUSState *pv = opaque;

    /* keep the register local for simple access */
    uint32_t src_reg = pv->src_control_reg[srcnum];

    if (level) {
        /* already set? */
        if (src_reg & IR_SRC_SRR) {
            return;
        }
        /* set SRR bit for requested interrupt */
        src_reg |= IR_SRC_SRR;

    } else {
        /* already cleared? */
        if (!(src_reg & IR_SRC_SRR)) {
            return;
        }
        /* set SRR bit for requested interrupt */
        src_reg &= ~IR_SRC_SRR;
    }
    /* write back modified register */
    pv->src_control_reg[srcnum] = src_reg;

    if (qemu_loglevel_mask(CPU_LOG_INT)) {
        qemu_log("tricore_irbus: SRC #%d (%s) level %d\n",
        srcnum, get_name_by_src(srcnum), level);
    }

    irq_evaluate(opaque);
}


static uint64_t tricore_irbus_srvcontrolregs_read(void *opaque, hwaddr offset,
        unsigned size)
{
    TriCoreIRBUSState *s = (TriCoreIRBUSState *) opaque;
    hwaddr reg_addr = offset >> 2;

    return s->src_control_reg[reg_addr];
}


static void tricore_irbus_srvcontrolregs_write(void *opaque, hwaddr offset,
        uint64_t value, unsigned size)
{
    TriCoreIRBUSState *s = (TriCoreIRBUSState *) opaque;
    int srcnum = -1;

    /* Since we can only write a byte, we have to calculate and shift
     some values. */
    hwaddr reg_addr = offset >> 2;

    switch (reg_addr) {
    case 0x20: /* SRC_ASCLINmTX offset: 0x80 */
        srcnum = IR_SRC_ASCLIN0TX;
        break;
    case 0x21: /* SRC_ASCLINmRX offset: 0x84 */
        srcnum = IR_SRC_ASCLIN0RX;
        break;
    case 0x22: /* SRC_ASCLINmEX offset: 0x88 */
        srcnum = IR_SRC_ASCLIN0EX;
        break;
    case 0x124: /* SRC_STMmSR0 0 offset: 0x490 */
        srcnum = IR_SRC_STM0_SR0;
        break;
    case 0x125: /* SRC_STMmSR1 0 offset: 0x494 */
        srcnum = IR_SRC_STM0_SR1;
        break;
    case 0x126: /* SRC_STMmSR0 1 offset: 0x498 */
        srcnum = IR_SRC_STM1_SR1;
        break;
    case 0x127: /* SRC_STMmSR1 1 offset: 0x49C */
        srcnum = IR_SRC_STM1_SR1;
        break;
    case 0x128: /* SRC_STMmSR0 2 offset: 0x4A0 */
        srcnum = IR_SRC_STM2_SR1;
        break;
    case 0x129: /* SRC_STMmSR1 2 offset: 0x4A4 */
        srcnum = IR_SRC_STM2_SR1;
        break;
    default:
        error_report(
                "tricore_stm_srvreq_write: write access to unknown register 0x"
                HWADDR_FMT_plx, offset);
        break;
    }

    if (srcnum >= 0) {
        uint32_t srcc = s->src_control_reg[srcnum];

        /* update the register content locally first */
        memcpy(((void *) &srcc) + (offset & 0x3), &value, size);

        /* handle SETR and CLRR bits */
        switch (srcc & (3 << 25)) {
        case 0:
        case 3:
            break;
        case 1:
            srcc &= ~(1 << 24);
            break;
        case 2:
            srcc |= (1 << 24);
            break;
        }
        srcc &= ~(3 << 25);

        s->src_control_reg[srcnum] = srcc;

        if (qemu_loglevel_mask(CPU_LOG_INT)) {
            qemu_log("tricore_irbus: SRC %s now %s (SRPN %d)\n",
            get_name_by_src(srcnum),
            (s->src_control_reg[srcnum] & (1 << 24)) ? "enabled" : "disabled",
            (s->src_control_reg[srcnum] & IR_SRC_SRPN));
        }
    }
}


static const MemoryRegionOps tricore_irbus_srvcontrolregs_ops = {
        .read = tricore_irbus_srvcontrolregs_read,
        .write = tricore_irbus_srvcontrolregs_write,
        .valid = {.min_access_size = 1, .max_access_size = 4, },
        .endianness = DEVICE_NATIVE_ENDIAN, };


static void tricore_irbus_init(Object *obj)
{
    TriCoreIRBUSState *pv = TRICORE_IRBUS(obj);

    qdev_init_gpio_in(DEVICE(pv), irq_handler, IR_SRC_COUNT);
    sysbus_init_irq(SYS_BUS_DEVICE(obj), &pv->parent_irq);
    memory_region_init_io(&pv->srvcontrolregs, OBJECT(pv),
            &tricore_irbus_srvcontrolregs_ops, pv, "tricore_irbus", 0x1FFF);
}

static void tricore_irbus_realize(DeviceState *dev, Error **errp)
{
    struct TriCoreIRBUSState *pv = TRICORE_IRBUS(dev);
    Error *err = NULL;

    pv->cpu = object_property_get_link(OBJECT(dev), "cpu", &err);
    if (!pv->cpu) {
        error_setg(errp, "tricore,irbus: CPU link not found: %s",
                error_get_pretty(err));
        return;
    }
}

static void tricore_irbus_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    /* Reason: needs to be wired up, e.g. by tricore_testboard_init() */
    dc->user_creatable = false;
    dc->realize = tricore_irbus_realize;
}

static TypeInfo tricore_irbus_info = { .name = "tricore_irbus", .parent =
        TYPE_SYS_BUS_DEVICE, .instance_size = sizeof(TriCoreIRBUSState),
        .instance_init = tricore_irbus_init, .class_init =
                tricore_irbus_class_init, };

static void tricore_irbus_register(void)
{
    type_register_static(&tricore_irbus_info);
}

type_init(tricore_irbus_register)



