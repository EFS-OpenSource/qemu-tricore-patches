/*
 * QEMU TriCore Debug device.
 *
 * Copyright (c) 2017 David Brenken
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#include "qemu/osdep.h"
#include "hw/sysbus.h"
#include "hw/tricore/tricore_virt.h"
#include <stdio.h>
#include <inttypes.h>
#include "qemu/log.h"

static void tricore_virt_write(void *opaque, hwaddr offset, uint64_t value,
        unsigned size)
{
    switch (offset) {
    case 0x0020:
        if (value != 0) {
            fputc((char) value, stdout);
        } else {
            fflush(stdout);
        }
        return;

    case 0x0024:
        if (value > 0) {
            usleep(1000 * value);
        } else {
            usleep(1);
        }
        return;

    case 0x0028:
        printf(
            "tricore_virt_write: Target code wants to exit emulator with return code %d\n",
            (uint32_t)value);
            exit(value);
        return;

    default:
        break;
    }
}

static uint64_t tricore_virt_read(void *opaque, hwaddr offset, unsigned size)
{
    switch (offset) {
    case 0x0000:
        return 0x00000100;

    case 0x0004:
        return 0x5533EE33;

    default:
        break;
    }

    return 0;
}

static const MemoryRegionOps tricore_virt_ops = {
	.read = tricore_virt_read,
    .write = tricore_virt_write,
	.valid = { .min_access_size = 1, .max_access_size = 4 },
	.endianness = DEVICE_NATIVE_ENDIAN,
};

static void tricore_virt_init(Object *obj)
{
    TriCoreVIRTState *s = TRICORE_VIRT(obj);
    /* map memory */
    memory_region_init_io(&s->iomem, OBJECT(s), &tricore_virt_ops, s,
            "tricore_virt", 0x00000040);
}

static const TypeInfo tricore_virt_info = {
	.name = TYPE_TRICORE_VIRT,
    .parent = TYPE_SYS_BUS_DEVICE,
	.instance_size = sizeof(TriCoreVIRTState),
	.instance_init = tricore_virt_init,
};

static void tricore_virt_register_types(void)
{
    type_register_static(&tricore_virt_info);
}

type_init(tricore_virt_register_types)
