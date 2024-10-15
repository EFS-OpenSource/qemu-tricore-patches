/*
 * QEMU TriCore Debug device.
 *
 * Copyright (c) 2017 David Brenken <david.brenken@efs-auto.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/tricore/tricore_sfr.h"
#include <stdio.h>
#include <inttypes.h>
#include "qemu/log.h"

static bool logging = false;

static void tricore_sfr_write(void *opaque, hwaddr offset, uint64_t value,
        unsigned size)
{
    TriCoreSFRState *s = (TriCoreSFRState *) opaque;
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

    /* did anything change? */
    if(s->regs[reg_addr] == value)
    {
        return;
    }

    uint32_t address = (uint32_t) (0xF0000000ULL + (reg_addr << 2));

#define LOCAL_LOG(name,desc) \
    if (logging) {\
        qemu_log("tricore_sfr_write: name: '%s', address 0x%X, value: 0x%X, desc: '%s'\n", \
            name, address, (uint32_t) value, desc);\
    }

    switch(address) {
//#include "tricore_sfr_tc27xd.inc"
    }

    s->regs[reg_addr] = value;

#undef LOCAL_LOG
}

static uint64_t tricore_sfr_read(void *opaque, hwaddr offset, unsigned size)
{
    TriCoreSFRState *s = (TriCoreSFRState *) opaque;
    hwaddr reg_addr = offset >> 2;
    uint64_t value = s->regs[reg_addr];

    uint32_t address = (uint32_t) (0xF0000000ULL + (reg_addr << 2));
#define LOCAL_LOG(name,desc) \
    if (logging) {\
        qemu_log("tricore_sfr_read: name: '%s', address 0x%X, desc: '%s'\n", \
            name, address, desc);\
    }

    switch(address) {
//#include "tricore_sfr_tc27xd.inc"
    }
    
#undef LOCAL_LOG
    value = value << ((offset & 0x3) * 0x8);

    if (size == 1) {
        value &= 0xFF;
    }
    if (size == 2) {
        value &= 0xFFFF;
    }
    return value;
}

static const MemoryRegionOps tricore_sfr_ops = {
	.read = tricore_sfr_read,
    .write = tricore_sfr_write,
	.valid = { .min_access_size = 1, .max_access_size = 4 },
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static void tricore_sfr_init(Object *obj)
{
    TriCoreSFRState *s = TRICORE_SFR(obj);
    /* map memory */
    memory_region_init_io(&s->iomem, OBJECT(s), &tricore_sfr_ops, s,
            "tricore_sfr", TRICORE_SFR_SIZE);

    /* when specified, custom execution tracing is enabled */
    if (getenv("TRICORE_SFR_LOGGING") != NULL) {
        qemu_log("Enable SFR logging\n");
        logging = true;
    }
}

static const TypeInfo tricore_sfr_info = {
	.name = TYPE_TRICORE_SFR,
    .parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(TriCoreSFRState),
	.instance_init = tricore_sfr_init,
};

static void tricore_sfr_register_types(void)
{
    type_register_static(&tricore_sfr_info);
}

type_init(tricore_sfr_register_types)
