/*
 * Infineon TriBoard System emulation.
 *
 * Copyright (c) 2020 Andreas Konopik
 * Copyright (c) 2020 David Brenken
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
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

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "hw/qdev-properties.h"
#include "net/net.h"
#include "hw/loader.h"
#include "elf.h"
#include "hw/tricore/tricore.h"
#include "qemu/error-report.h"
#include "qemu/log.h"
#include "qemu/option.h"
#include "qemu/config-file.h"

#include "hw/tricore/triboard.h"

static void tricore_load_kernel(const char *kernel_filename)
{
    TriCoreCPU *cpu;
    CPUTriCoreState *env;
    uint64_t entry;
    ssize_t kernel_size;

    char *file = strdup(kernel_filename);
    char *cur_file = file;

    cpu = TRICORE_CPU(first_cpu);
    env = &cpu->env;

    while (cur_file) {
        char *comma = strchr(cur_file, ',');

        if (comma) {
            *comma = '\0';
        }
        qemu_log("Loading ELF '%s'\n", cur_file);

        kernel_size = load_elf(cur_file, NULL, NULL, NULL, &entry, NULL, NULL,
                NULL, 0, EM_TRICORE, 1, 0);
        if (kernel_size <= 0) {
            error_report("qemu: no kernel file '%s'", cur_file);
            exit(1);
        }
        if (!env->PC) {
            env->PC_entry = entry;
        }

        if (comma) {
            cur_file = comma + 1;
        } else {
            cur_file = NULL;
        }
    }
}

/* return 1 if rom was loaded into BROM, return 0 in case of a failure */
static int tricore_load_brom(MemoryRegion *boot_rom)
{
    /* Check for "option-rom" commandline option to map romfile to BROM */
    QemuOptsList *plist = qemu_find_opts("option-rom");
    QemuOpts *opts = QTAILQ_FIRST(&plist->head);
    const char *romfile;
    int64_t romsize;

    romfile = qemu_opt_get(opts, "romfile");

    if (romfile) {
        qemu_log("Loading BootROM '%s'\n", romfile);
        romsize = get_image_size(romfile);
        if (romsize == -1) {
            error_report("Cannot read ROM file %s", romfile);
            return 0;
        }

        rom_add_file_mr(romfile, boot_rom, -1);

        return 1;
    }
    return 0;
}

static void triboard_machine_tc39xb_init(MachineState *machine)
{
    TriBoardMachineState *ms = TRIBOARD_MACHINE(machine);
    TriBoardMachineClass *amc = TRIBOARD_MACHINE_GET_CLASS(machine);

    object_initialize_child(OBJECT(machine), "tc39xb_soc", &ms->tc39xb_soc, amc->soc_name);
    sysbus_realize(SYS_BUS_DEVICE(&ms->tc39xb_soc), &error_fatal);

    if (machine->kernel_filename) {
        tricore_load_kernel(machine->kernel_filename);
    }

    MemoryRegion *brom = &ms->tc39xb_soc.flashmem.brom_c;
    if (tricore_load_brom(brom)) {
        TriCoreCPU *cpu = TRICORE_CPU(first_cpu);
        CPUTriCoreState *env = &cpu->env;
        env->PC_entry = brom->addr;
    }
}

static void triboard_machine_tc27xd_init(MachineState *machine)
{
    TriBoardMachineState *ms = TRIBOARD_MACHINE(machine);
    TriBoardMachineClass *amc = TRIBOARD_MACHINE_GET_CLASS(machine);

    object_initialize_child(OBJECT(machine), "tc27xd_soc", &ms->tc27xd_soc, amc->soc_name);
    sysbus_realize(SYS_BUS_DEVICE(&ms->tc27xd_soc), &error_fatal);

    if (machine->kernel_filename) {
        tricore_load_kernel(machine->kernel_filename);
    }

    MemoryRegion *brom = &ms->tc27xd_soc.flashmem.brom_c;
    if (tricore_load_brom(brom)) {
        TriCoreCPU *cpu = TRICORE_CPU(first_cpu);
        CPUTriCoreState *env = &cpu->env;
        env->PC_entry = brom->addr;
    }
}

static void triboard_machine_tc277d_class_init(ObjectClass *oc,
        void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    TriBoardMachineClass *amc = TRIBOARD_MACHINE_CLASS(oc);

    mc->init        = triboard_machine_tc27xd_init;
    mc->desc        = "Infineon AURIX TriBoard TC277 (D-Step)";
    mc->max_cpus    = 1;
    amc->soc_name   = "tc277d-soc";
};

static void triboard_machine_tc397b_class_init(ObjectClass *oc,
        void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    TriBoardMachineClass *amc = TRIBOARD_MACHINE_CLASS(oc);

    mc->init        = triboard_machine_tc39xb_init;
    mc->desc        = "Infineon AURIX TriBoard TC397 (B-Step)";
    mc->max_cpus    = 1;
    amc->soc_name   = "tc397b-soc";
};

static const TypeInfo triboard_machine_types[] = {
    {
        .name           = TYPE_TRIBOARD_MACHINE,
        .parent         = TYPE_MACHINE,
        .instance_size  = sizeof(TriBoardMachineState),
        .class_size     = sizeof(TriBoardMachineClass),
        .abstract       = true,
    }, {
        .name           = MACHINE_TYPE_NAME("KIT_AURIX_TC277D_TRB"),
        .parent         = TYPE_TRIBOARD_MACHINE,
        .class_init     = triboard_machine_tc277d_class_init,
    }, {
        .name           = MACHINE_TYPE_NAME("KIT_AURIX_TC397B_TRB"),
        .parent         = TYPE_TRIBOARD_MACHINE,
        .class_init     = triboard_machine_tc397b_class_init,
    }, 
};

DEFINE_TYPES(triboard_machine_types)
