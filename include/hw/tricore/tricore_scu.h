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

#ifndef HW_TRICORE_SCU_H
#define HW_TRICORE_SCU_H

#include "hw/sysbus.h"
#include "hw/hw.h"
#include "hw/irq.h"

#define TYPE_TRICORE_SCU "tricore_scu"
#define TRICORE_SCU(obj) \
   OBJECT_CHECK(TriCoreSCUState, (obj), TYPE_TRICORE_SCU)

#define MASK_OSCCON_PLLLV 0x00000001
#define MASK_OSCCON_PLLHV 0x00000100
#define MASK_PLLCON0_VCOBYP 0x00000001
#define MASK_PLLCON0_SETFINDIS 0x00000010
#define MASK_PLLCON1_K1DIV 0x007F0000
#define MASK_PLLCON1_K2DIV 0x0000003F
#define MASK_PLLCON1_K3DIV 0x00007F00
#define MASK_CCUCON0_UP 0x40000000
#define MASK_CCUCON1_UP 0x40000000
#define MASK_CCUCON5_UP 0x40000000
#define MASK_CCUCON1_STMDIV 0x00000F00
#define MASK_CCUCON1_INSEL 0x30000000
#define MASK_CCUCON1_INSEL_BACKUP   0x0
#define MASK_CCUCON1_INSEL_OSC0   0x10000000
#define MASK_PLLCON0_NDIV   0x0000FE00
#define MASK_PLLCON0_PDIV   0x0F000000
#define MASK_PLLCON0_VCOBYP 0x00000001
#define MASK_PLLCON0_SETFINDIS 0x00000010
#define MASK_PLLCON0_CLRFINDIS 0x00000020
#define MASK_CCUCON0_SRIDIV 0x00000F00
#define MASK_CCUCON0_SPBDIV 0x000F0000
#define MASK_PLLSTAT_VCOBYST 0x1
#define MASK_PLLSTAT_VCOLOCK 0x00000004
#define MASK_PLLSTAT_FINDIS 0x00000008
#define SCU_FBACKUP 100000000
#define SCU_XTAL1 20000000

/* reset values */
#define RESET_TRICORE_OSCCON 0x00000112
#define RESET_TRICORE_PLLSTAT 0x00000038
#define RESET_TRICORE_PLLCON0 0x0001C600
#define RESET_TRICORE_PLLCON1 0x0002020F
#define RESET_TRICORE_PLLCON2 0x0
#define RESET_TRICORE_PLLERAYSTAT 0x00000038
#define RESET_TRICORE_PLLERAYCON0 0x00012E00
#define RESET_TRICORE_PLLERAYCON1 0x000F020F
#define RESET_TRICORE_CCUCON0 0x01120148
#define RESET_TRICORE_CCUCON1 0x00002211
#define RESET_TRICORE_FDR 0x0
#define RESET_TRICORE_EXTCON 0x0
#define RESET_TRICORE_CCUCON2 0x00000002
#define RESET_TRICORE_CCUCON3 0x0
#define RESET_TRICORE_CCUCON4 0x0
#define RESET_TRICORE_CCUCON5 0x00000041
#define RESET_TRICORE_CCUCON6 0x0
#define RESET_TRICORE_CCUCON7 0x0
#define RESET_TRICORE_CCUCON8 0x0
#define RESET_TRICORE_WDTSCON0 0xFFFC000E
#define RESET_TRICORE_WDTSCON1 0x0
#define RESET_TRICORE_WDTCPU0CON0 0xFFFC000E

typedef enum {
    TRICORE_SCU_NORMAL, TRICORE_SCU_FREERUNNING, TRICORE_SCU_PRESCALER
} TriCore_SCU_Mode_Type;

typedef struct {
    /* <private> */
    SysBusDevice parent_obj;
    Object *cpu;
    qemu_irq reset_line;

    /* <public> */
    MemoryRegion iomem;
    TriCore_SCU_Mode_Type mode;

    /* CCU registers */
    uint32_t OSCCON;
    uint32_t PLLSTAT;
    uint32_t PLLCON[3];
    uint32_t PLLERAYSTAT;
    uint32_t PLLERAYCON[2];
    uint32_t CCUCON[9];
    uint32_t FDR;
    uint32_t EXTCON;

    /* SCU registers */
    uint32_t WDTSCON0;
    uint32_t WDTSCON1;
    uint32_t WDTCPU0CON0;

} TriCoreSCUState;

uint32_t tricore_scu_get_stmclock(TriCoreSCUState *s);
uint32_t tricore_scu_get_sri_clock(TriCoreSCUState *s);
uint32_t tricore_scu_get_spbclock(TriCoreSCUState *s);



#endif

