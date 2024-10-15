/*
 * Infineon tc39x SoC System emulation.
 *
 * Copyright (c) 2020 Andreas Konopik <andreas.konopik@efs-auto.de>
 * Copyright (c) 2020 David Brenken <david.brenken@efs-auto.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TC39XB_SOC_H
#define TC39XB_SOC_H

#include "hw/sysbus.h"
#include "target/tricore/cpu.h"
#include "qom/object.h"

#include "hw/tricore/tricore.h"
#include "hw/tricore/tricore_virt.h"
#include "hw/tricore/tricore_ir.h"
#include "hw/tricore/tricore_scu.h"
#include "hw/tricore/tricore_sfr.h"
#include "hw/intc/tricore_irbus.h"
#include "hw/timer/tricore_stm.h"
#include "hw/char/tricore_asclin.h"
#include "hw/tricore/tc_soc.h"

#define TYPE_TC39XB_SOC ("tc39xb-soc")
OBJECT_DECLARE_TYPE(TC39XBSoCState, TC39XBSoCClass, TC39XB_SOC)

typedef struct TC39XBSoCCPUMemState {

    MemoryRegion dspr;
    MemoryRegion pspr;

    MemoryRegion dcache;
    MemoryRegion dtag;
    MemoryRegion pcache;
    MemoryRegion ptag;

    MemoryRegion pflash_c;
    MemoryRegion pflash_u;
    MemoryRegion dlmu_c;
    MemoryRegion dlmu_u;

} TC39XBSoCCPUMemState;

#define TC39XB_MEMDEV_CPU(n) \
    TC39XB_DSPR##n,      \
    TC39XB_DCACHE##n,    \
    TC39XB_DTAG##n,      \
    TC39XB_PSPR##n,      \
    TC39XB_PCACHE##n,    \
    TC39XB_PTAG##n,      \
    TC39XB_DLMU##n##_U,  \
    TC39XB_DLMU##n##_C,  \
    TC39XB_PFLASH##n##_U,\
    TC39XB_PFLASH##n##_C
    


typedef struct TC39XBSoCFlashMemState {
    MemoryRegion dflash0;
    MemoryRegion dflash1;
    MemoryRegion olda_c;
    MemoryRegion olda_u;
    MemoryRegion brom_c;
    MemoryRegion brom_u;
    MemoryRegion lmu0_c;
    MemoryRegion lmu0_u;
    MemoryRegion lmu1_c;
    MemoryRegion lmu1_u;
    MemoryRegion lmu2_c;
    MemoryRegion lmu2_u;
    MemoryRegion emem;

} TC39XBSoCFlashMemState;

typedef struct TC39XBSoCState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    TriCoreCPU cpu;

    MemoryRegion dsprX;
    MemoryRegion psprX;

    TC39XBSoCCPUMemState cpu0mem;
    TC39XBSoCCPUMemState cpu1mem;
    TC39XBSoCCPUMemState cpu2mem;
    TC39XBSoCCPUMemState cpu3mem;
    TC39XBSoCCPUMemState cpu4mem;
    TC39XBSoCCPUMemState cpu5mem;
    TC39XBSoCFlashMemState flashmem;
    
    TriCoreIRBUSState *irbus;
    TriCoreVIRTState *virt;
    TriCoreSCUState *scu;
    TriCoreSTMState *stm;
    TriCoreSFRState *sfr;
    TriCoreASCLINState *asclin;

    qemu_irq irq[IR_SRC_COUNT];
    qemu_irq *cpu_irq;


} TC39XBSoCState;

typedef struct TC39XBSoCClass {
    DeviceClass parent_class;

    const char *name;
    const char *cpu_type;
    const MemmapEntry *memmap;
    uint32_t num_cpus;
} TC39XBSoCClass;

enum {
    TC39XB_MEMDEV_CPU(5),
    TC39XB_MEMDEV_CPU(4),
    TC39XB_MEMDEV_CPU(3),
    TC39XB_MEMDEV_CPU(2),
    TC39XB_MEMDEV_CPU(1),
    TC39XB_MEMDEV_CPU(0),
    
    TC39XB_OLDA_C,
    TC39XB_OLDA_U,
    TC39XB_BROM_C,
    TC39XB_BROM_U,

    TC39XB_LMU0_C,
    TC39XB_LMU1_C,
    TC39XB_LMU2_C,
    TC39XB_LMU0_U,
    TC39XB_LMU1_U,
    TC39XB_LMU2_U,

    TC39XB_EMEM,

    TC39XB_DFLASH0,
    TC39XB_DFLASH1,
    TC39XB_PSPRX,
    TC39XB_DSPRX,
    TC39XB_VIRT,
    TC39XB_SFR,
    TC39XB_IRBUS,
    TC39XB_SCU,
    TC39XB_STM,
    TC39XB_ASCLIN,
};

#endif
