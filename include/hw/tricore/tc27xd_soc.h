/*
 * Infineon tc27x SoC System emulation.
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

#ifndef TC27XD_SOC_H
#define TC27XD_SOC_H

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

#define TYPE_TC27XD_SOC ("tc27xd-soc")
OBJECT_DECLARE_TYPE(TC27XDSoCState, TC27XDSoCClass, TC27XD_SOC)

typedef struct TC27XDSoCCPUMemState {

    MemoryRegion dspr;
    MemoryRegion pspr;

    MemoryRegion dcache;
    MemoryRegion dtag;
    MemoryRegion pcache;
    MemoryRegion ptag;

} TC27XDSoCCPUMemState;

typedef struct TC27XDSoCFlashMemState {

    MemoryRegion pflash0_c;
    MemoryRegion pflash1_c;
    MemoryRegion pflash0_u;
    MemoryRegion pflash1_u;
    MemoryRegion dflash0;
    MemoryRegion dflash1;
    MemoryRegion olda_c;
    MemoryRegion olda_u;
    MemoryRegion brom_c;
    MemoryRegion brom_u;
    MemoryRegion lmuram_c;
    MemoryRegion lmuram_u;
    MemoryRegion emem_c;
    MemoryRegion emem_u;

} TC27XDSoCFlashMemState;

typedef struct TC27XDSoCState {
    /*< private >*/
    SysBusDevice parent_obj;

    /*< public >*/
    TriCoreCPU cpu;

    MemoryRegion dsprX;
    MemoryRegion psprX;

    TC27XDSoCCPUMemState cpu0mem;
    TC27XDSoCCPUMemState cpu1mem;
    TC27XDSoCCPUMemState cpu2mem;
    
    TriCoreIRBUSState *irbus;
    TriCoreVIRTState *virt;
    TriCoreSCUState *scu;
    TriCoreSTMState *stm;
    TriCoreASCLINState *asclin;
    TriCoreSFRState *sfr;

    qemu_irq irq[IR_SRC_COUNT];
    qemu_irq *cpu_irq;

    TC27XDSoCFlashMemState flashmem;

} TC27XDSoCState;

typedef struct TC27XDSoCClass {
    DeviceClass parent_class;

    const char *name;
    const char *cpu_type;
    const MemmapEntry *memmap;
    uint32_t num_cpus;
} TC27XDSoCClass;

#define TC27XD_MEMDEV_CPU(n) \
    TC27XD_DSPR##n,   \
    TC27XD_DCACHE##n, \
    TC27XD_DTAG##n,   \
    TC27XD_PSPR##n,   \
    TC27XD_PCACHE##n, \
    TC27XD_PTAG##n

enum {
    TC27XD_MEMDEV_CPU(2),
    TC27XD_MEMDEV_CPU(1),
    TC27XD_MEMDEV_CPU(0),
    TC27XD_PFLASH0_C,
    TC27XD_PFLASH1_C,
    TC27XD_OLDA_C,
    TC27XD_BROM_C,
    TC27XD_LMURAM_C,
    TC27XD_EMEM_C,
    TC27XD_PFLASH0_U,
    TC27XD_PFLASH1_U,
    TC27XD_DFLASH0,
    TC27XD_DFLASH1,
    TC27XD_OLDA_U,
    TC27XD_BROM_U,
    TC27XD_LMURAM_U,
    TC27XD_EMEM_U,
    TC27XD_PSPRX,
    TC27XD_DSPRX,
    TC27XD_SFR,
    TC27XD_VIRT,
    TC27XD_IRBUS,
    TC27XD_SCU,
    TC27XD_STM,
    TC27XD_ASCLIN,
};

#endif
