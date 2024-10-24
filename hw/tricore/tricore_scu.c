/*
 * QEMU TriCore SCU device.
 *
 * Copyright (c) 2017 David Brenken
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "hw/sysbus.h"
#include "hw/qdev-properties.h"
#include "hw/tricore/tricore_scu.h"
#include "target/tricore/cpu.h"
#include <stdio.h>
#include "qemu/error-report.h"
#include <inttypes.h>
#include "qemu/log.h"
#include <math.h>

static uint32_t tricore_scu_get_fOsc(TriCoreSCUState *s)
{
    uint32_t fOsc = 0x0;

    /* Get clock input. */
    if ((s->CCUCON[1] & MASK_CCUCON1_INSEL) == MASK_CCUCON1_INSEL_BACKUP) {
        fOsc = SCU_FBACKUP; /* Back-up clock. */

    } else if ((s->CCUCON[1] & MASK_CCUCON1_INSEL) == MASK_CCUCON1_INSEL_OSC0) {
        fOsc = SCU_XTAL1; /* External oscillator. */
    }

    return fOsc;
}

static uint32_t tricore_scu_get_fPLL(TriCoreSCUState *s)
{
    uint32_t fOsc = tricore_scu_get_fOsc(s);
    uint32_t fPLL = 0x0;

    if ((s->CCUCON[1] & MASK_CCUCON1_INSEL) == MASK_CCUCON1_INSEL_BACKUP) {
        /* Backup mode. */
        return fOsc;
    } else if (((s->PLLSTAT & MASK_PLLSTAT_FINDIS >> 3) == 0x0)
            && ((s->PLLSTAT & MASK_PLLSTAT_VCOBYST) == 0x0)
            && (((s->PLLSTAT & MASK_PLLSTAT_VCOLOCK) >> 2) == 0x1)
            && (((s->OSCCON & MASK_OSCCON_PLLHV) >> 8) == 0x1)
            && ((s->OSCCON & MASK_OSCCON_PLLLV) == 0x1)) { /* Normal mode. */
        uint32_t pdiv = ((s->PLLCON[0] & MASK_PLLCON0_PDIV) >> 24) + 1;
        uint32_t ndiv = ((s->PLLCON[0] & MASK_PLLCON0_NDIV) >> 9) + 1;
        uint32_t k2div = (s->PLLCON[1] & MASK_PLLCON1_K2DIV) + 1;

        fPLL = (ndiv * fOsc) / (pdiv * k2div);
    } else if (((s->PLLSTAT & MASK_PLLSTAT_VCOBYST) == 0x1)) {
        /* Prescaler mode. */
        uint32_t k1div = ((s->PLLCON[1] & MASK_PLLCON1_K1DIV) >> 16) + 1;
        fPLL = fOsc / k1div;

    } else if (((s->PLLSTAT & MASK_PLLSTAT_VCOBYST) == 0x0)
            && (((s->PLLSTAT & MASK_PLLSTAT_FINDIS) >> 3) == 0x1)) {
        /* Freerunning mode. */
        /* ToDo */
        fPLL = 0x0;
        error_report("TriCore SCU: Freerunning mode is not implemented.");
    } else {
        error_report("TriCore SCU: illegal configuration");
    }
    return fPLL;
}

static uint8_t tricore_scu_get_stmdiv(TriCoreSCUState *s)
{
    uint8_t stmdiv = (uint8_t) ((s->CCUCON[1] & MASK_CCUCON1_STMDIV) >> 8);

    /* Input check. */
    switch (stmdiv) {
    case 7:
    case 9:
    case 11:
    case 13:
    case 14:
        error_report("tricore_scu STMDIV has undefined value");
        break;
    default:
        break;
    }
    return stmdiv;
}

static uint8_t tricore_scu_get_sridiv(TriCoreSCUState *s)
{
    uint8_t sridiv = (uint8_t) ((s->CCUCON[0] & MASK_CCUCON0_SRIDIV) >> 8);

    /* Input check. */
    switch (sridiv) {
    case 7:
    case 9:
    case 11:
    case 13:
    case 14:
        error_report("TriCore SCU: SRIDIV has undefined value");
        break;
    default:
        break;
    }
    return sridiv;
}

static uint8_t tricore_scu_get_spbdiv(TriCoreSCUState *s)
{
    uint8_t spbdiv = (uint8_t) ((s->CCUCON[0] & MASK_CCUCON0_SPBDIV) >> 16);

    /* Input check. */
    switch (spbdiv) {
    case 7:
    case 9:
    case 11:
    case 13:
    case 14:
        error_report("TriCore SCU: SPBDIV has undefined value");
        break;
    default:
        break;
    }
    return spbdiv;
}

uint32_t tricore_scu_get_stmclock(TriCoreSCUState *s)
{
    uint32_t stmdiv = tricore_scu_get_stmdiv(s);
    uint32_t fPLL = tricore_scu_get_fPLL(s);

    return fPLL / (stmdiv);
}

uint32_t tricore_scu_get_spbclock(TriCoreSCUState *s)
{
    uint32_t spbdiv = tricore_scu_get_spbdiv(s);
    uint32_t fPLL = tricore_scu_get_fPLL(s);
    return fPLL / (spbdiv);
}

uint32_t tricore_scu_get_sri_clock(TriCoreSCUState *s)
{
    uint32_t sridiv = tricore_scu_get_sridiv(s);
    uint32_t fPLL = tricore_scu_get_fPLL(s);
    return fPLL / (sridiv);
}

static void tricore_scu_update_mode(TriCoreSCUState *s)
{
    if (((s->PLLCON[0] & MASK_PLLCON0_VCOBYP) == 0x0)
            && (((s->PLLSTAT & MASK_PLLCON0_CLRFINDIS) >> 5) == 0x1)) {
        /* Normal mode. */
        s->mode = TRICORE_SCU_NORMAL;

        /* Unset PLLSTAT VCOBYST. */
        s->PLLSTAT = (s->PLLSTAT & ~MASK_PLLSTAT_VCOBYST);
    } else if (((s->PLLCON[0] & MASK_PLLCON0_VCOBYP) == 0x1)) {
        /* Prescaler mode. */
        s->mode = TRICORE_SCU_PRESCALER;

        /* Set PLLSTAT VCOBYST. */
        s->PLLSTAT = s->PLLSTAT | MASK_PLLSTAT_VCOBYST;
    } else if (((s->PLLCON[0] & MASK_PLLCON0_VCOBYP) == 0x0)
            && (((s->PLLCON[0] & MASK_PLLCON0_SETFINDIS) >> 4) == 0x1)) {
        /* Freerunning mode. */
        s->mode = TRICORE_SCU_FREERUNNING;

        /* Unset PLLSTAT VCOBYST. */
        s->PLLSTAT = (s->PLLSTAT & ~MASK_PLLSTAT_VCOBYST);

        error_report("TriCore SCU: Freerunning mode is not implemented.");
    } else {
        error_report("TriCore SCU: illegal configuration");
    }
}

static void tricore_scu_establish_configuration(TriCoreSCUState *s)
{
    if ((s->CCUCON[0] & MASK_CCUCON0_UP) || (s->CCUCON[1] & MASK_CCUCON1_UP)
            || (s->CCUCON[5] & MASK_CCUCON5_UP)) {
        /* Erase UP bit. */
        s->CCUCON[0] = (s->CCUCON[0] & ~MASK_CCUCON0_UP);
        s->CCUCON[1] = (s->CCUCON[1] & ~MASK_CCUCON1_UP);
        s->CCUCON[5] = (s->CCUCON[5] & ~MASK_CCUCON5_UP);
    }
}

static void tricore_scu_write(void *opaque, hwaddr offset, uint64_t value,
        unsigned size)
{
    TriCoreSCUState *s = (TriCoreSCUState *) opaque;

    hwaddr reg_addr = offset >> 2;
    reg_addr = reg_addr << 2;

    switch (reg_addr) {
    case 0x18:
        memcpy((((char *) &(s->PLLCON[0]))) + (offset & 0x3), &value, size);

        if (s->PLLCON[0] & MASK_PLLCON0_SETFINDIS) {
            s->PLLSTAT |= MASK_PLLSTAT_FINDIS;
        }

        if (s->PLLCON[0] & MASK_PLLCON0_CLRFINDIS) {
            s->PLLSTAT = (s->PLLSTAT & ~MASK_PLLSTAT_FINDIS);
        }

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x1C:
        memcpy(&(s->PLLCON[1]) + (offset & 0x3), &value, size);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x20:
        memcpy(&(s->PLLCON[2]) + (offset & 0x3), &value, size);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x30: /* CCUCON0 */
        memcpy(&(s->CCUCON[0]) + (offset & 0x3), &value, size);

        tricore_scu_establish_configuration(s);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x34: /* CCUCON1 */
        memcpy(&(s->CCUCON[1]) + (offset & 0x3), &value, size);

        tricore_scu_establish_configuration(s);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x40:
        memcpy(&(s->CCUCON[2]) + (offset & 0x3), &value, size);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x44:
        memcpy(&(s->CCUCON[3]) + (offset & 0x3), &value, size);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x48:
        memcpy(&(s->CCUCON[4]) + (offset & 0x3), &value, size);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x4c: /* CCUCON5 */
        memcpy(&(s->CCUCON[5]) + (offset & 0x3), &value, size);

        tricore_scu_establish_configuration(s);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
        
    case 0x60: /* SWRSTCON */
        if(value & 2) {
            CPUTriCoreState *env = &((TriCoreCPU *) (s->cpu))->env;
            env->reset_pending = 1;
            
            qemu_log("tricore_scu_write: Software reset requested\n");
            qemu_irq_raise(s->reset_line);
        }
        break;
    case 0x80:
        memcpy(&(s->CCUCON[6]) + (offset & 0x3), &value, size);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x84:
        memcpy(&(s->CCUCON[7]) + (offset & 0x3), &value, size);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0x88:
        memcpy(&(s->CCUCON[8]) + (offset & 0x3), &value, size);

        /* Workaround: Normally it should take a while to raise VCOLOCK. */
        s->PLLSTAT |= MASK_PLLSTAT_VCOLOCK;
        break;
    case 0xf0:
        /* SCU_WDTS_CON0 */
        memcpy(&(s->WDTSCON1) + (offset & 0x3), &value, size);
        break;
    case 0x100:
        memcpy(&(s->WDTCPU0CON0) + (offset & 0x3), &value, size);
        break;
    default:
        break;
    }
    tricore_scu_update_mode(s);
}

static uint64_t tricore_scu_read(void *opaque, hwaddr offset, unsigned size)
{
    TriCoreSCUState *s = (TriCoreSCUState *) opaque;

    /* offset
     * 0x35 is CCUCON1: STMDIV
     * 0x10 is OSCCON: PLLLV and some more fields
     * 0x11 is OSCCON: PLLHV and some more fields
     * 0x14 is PLLSTAT: K1RDY, K2RDY,VCOLOCK, VCOBYST, FINDIS and some more
     * fields
     */

    /* Since we can only write a byte, we have to calculate
     and shift some values. */
    hwaddr reg_addr = offset >> 2;
    reg_addr = reg_addr << 2;

    uint64_t r;

    switch (reg_addr) {
    case 0x0:
        r = 0x0;
        break;
    case 0x10:
        r = s->OSCCON;
        break;
    case 0x14:
        if (s->PLLCON[0] & MASK_PLLCON0_VCOBYP) {
            r = 0b000110101;
        } else {
            r = 0b000110100;
        }
        r = s->PLLSTAT;
        break;
    case 0x18:
        r = s->PLLCON[0];
        break;
    case 0x1C:
        r = s->PLLCON[1];
        break;
    case 0x20:
        r = s->PLLCON[2];
        break;
    case 0x30:
        r = s->CCUCON[0];
        break;
    case 0x34:
        r = s->CCUCON[1];
        break;
    case 0xf0:
        /* SCU_WDTS_CON0 */
        r = s->WDTSCON1;
        break;
    case 0x100:
        r = s->WDTCPU0CON0;
        break;
    case 0x140:
        r = 0x47477172 | (1 << 31);
        break;
    default:
        r = 0x0;
        break;
    }

    if (reg_addr != offset) {
        /* Mask the return value. */
        int shift_val = (offset - reg_addr) * 8;
        int mask = (pow(2, 8 * size)) - 1;
        mask = mask << shift_val;

        /* Shift the return value according to the reading address. */
        r = r & mask;
        r = r >> shift_val;
    }

    return r;
}

static void tricore_scu_reset(DeviceState *dev)
{
    TriCoreSCUState *s = (TriCoreSCUState *) dev;

    s->CCUCON[0] = RESET_TRICORE_CCUCON0;
    s->CCUCON[1] = RESET_TRICORE_CCUCON1;
    s->CCUCON[2] = RESET_TRICORE_CCUCON2;
    s->CCUCON[3] = RESET_TRICORE_CCUCON3;
    s->CCUCON[4] = RESET_TRICORE_CCUCON4;
    s->CCUCON[5] = RESET_TRICORE_CCUCON5;
    s->CCUCON[6] = RESET_TRICORE_CCUCON6;
    s->CCUCON[7] = RESET_TRICORE_CCUCON7;
    s->CCUCON[8] = RESET_TRICORE_CCUCON8;
    s->EXTCON = RESET_TRICORE_EXTCON;
    s->FDR = RESET_TRICORE_FDR;
    s->OSCCON = RESET_TRICORE_OSCCON;
    /* Set PLLLV and PLLHV to indicate that the hardware is ready. */
    s->OSCCON = (s->OSCCON | MASK_OSCCON_PLLHV | MASK_OSCCON_PLLLV);
    s->PLLCON[0] = RESET_TRICORE_PLLCON0;
    s->PLLCON[1] = RESET_TRICORE_PLLCON1;
    s->PLLCON[2] = RESET_TRICORE_PLLCON2;
    s->PLLERAYCON[0] = RESET_TRICORE_PLLERAYCON0;
    s->PLLERAYCON[1] = RESET_TRICORE_PLLERAYCON1;
    s->PLLERAYSTAT = RESET_TRICORE_PLLERAYSTAT;
    s->PLLSTAT = RESET_TRICORE_PLLSTAT;
    /* Enable VCO lock should be given. */
    s->PLLSTAT = (s->PLLSTAT & ~MASK_PLLSTAT_VCOLOCK) | MASK_PLLSTAT_VCOLOCK;
    /* Disable FINDIS. */
    s->PLLSTAT = (s->PLLSTAT & ~MASK_PLLSTAT_FINDIS) | 0x0;

    s->WDTCPU0CON0 = RESET_TRICORE_WDTCPU0CON0;
    s->WDTSCON0 = RESET_TRICORE_WDTSCON0;
    s->WDTSCON1 = RESET_TRICORE_WDTSCON1;
}

static const MemoryRegionOps tricore_scu_ops = {
    .read = tricore_scu_read,
    .write = tricore_scu_write,
    .valid = { .min_access_size = 1, .max_access_size = 4, }, 
    .endianness = DEVICE_NATIVE_ENDIAN
};

static void tricore_scu_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    TriCoreSCUState *s = TRICORE_SCU(obj);

    tricore_scu_reset((DeviceState *) s);

    /* map memory */
    memory_region_init_io(&s->iomem, OBJECT(s), &tricore_scu_ops, s, "tricore_scu", 0x400);
    
    sysbus_init_irq(sbd, &s->reset_line);
}

static void tricore_scu_realize(DeviceState *dev, Error **errp)
{
    TriCoreSCUState *s = TRICORE_SCU(dev);
    Error *err = NULL;

    s->cpu = object_property_get_link(OBJECT(dev), "cpu", &err);
    if (!s->cpu) {
        error_setg(errp, "tricore_scu: CPU link not found: %s",
                error_get_pretty(err));
        return;
    }
}

static Property tricore_scu_properties[] = { DEFINE_PROP_END_OF_LIST() };

static void tricore_scu_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    device_class_set_props(dc, tricore_scu_properties);
    dc->reset = tricore_scu_reset;
    dc->realize = tricore_scu_realize;
}

static const TypeInfo tricore_scu_info = {
        .name = TYPE_TRICORE_SCU, .parent = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(TriCoreSCUState), .instance_init =
                tricore_scu_init, .class_init = tricore_scu_class_init, };

static void tricore_scu_register_types(void)
{
    type_register_static(&tricore_scu_info);
}

type_init(tricore_scu_register_types)
