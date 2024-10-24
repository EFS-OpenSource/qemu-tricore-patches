/*
 * QEMU TriCore Interrupt Router Bus.
 *
 * Copyright (c) 2017 David Brenken
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef HW_TRICORE_IRBUS_H
#define HW_TRICORE_IRBUS_H


#define TYPE_TRICORE_IRBUS "tricore_irbus"
#define TRICORE_IRBUS(obj) \
    OBJECT_CHECK(TriCoreIRBUSState, (obj), TYPE_TRICORE_IRBUS)

#define IR_SRC_COUNT 256

#define IR_SRC_SRPN 0xFF

#define IR_SRC_SRE  (1 << 10)
#define IR_SRC_SRR  (1 << 24)

/* those are the index entries into interrupt router */
#define IR_SRC_ASCLIN0TX     9
#define IR_SRC_ASCLIN0RX    10
#define IR_SRC_ASCLIN0EX    11
#define IR_SRC_STM0_SR0    103
#define IR_SRC_STM0_SR1    104
#define IR_SRC_STM1_SR0    105
#define IR_SRC_STM1_SR1    106
#define IR_SRC_STM2_SR0    107
#define IR_SRC_STM2_SR1    108
#define IR_SRC_RESET       254



typedef struct TriCoreIRBUSState {
    SysBusDevice parent_obj;
    void *cpu;
    MemoryRegion srvcontrolregs;
    uint32_t src_control_reg[IR_SRC_COUNT];
    uint8_t interruptstatusregs[IR_SRC_COUNT];
    qemu_irq parent_irq;
} TriCoreIRBUSState;

#endif
