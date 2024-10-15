/*
 * QEMU TriCore ASCLIN device.
 *
 * Copyright (c) 2017 David Brenken <david.brenken@efs-auto.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */

#ifndef ASCLIN_UART_H
#define ASCLIN_UART_H

#include "hw/sysbus.h"
#include "qapi/error.h"
#include "hw/irq.h"
#include "hw/qdev-properties.h"
#include "chardev/char-fe.h"
#include "hw/ptimer.h"

enum {
    STAT_THRE = (1 << 0), STAT_RX_EVT = (1 << 1), STAT_TX_EVT = (1 << 2),
};

enum {
    CTRL_RX_IRQ_EN = (1 << 0),
    CTRL_TX_IRQ_EN = (1 << 1),
    CTRL_THRU_EN = (1 << 2),
};

enum {
    DBG_BREAK_EN = (1 << 0),
};

#define MASK_FLAGS_TH 0x1
#define MASK_FLAGS_TR 0x2
#define MASK_FLAGS_RH 0x4
#define MASK_FLAGS_RR 0x8
#define MASK_FLAGS_PE 0x00010000
#define MASK_FLAGS_TC 0x00020000
#define MASK_FLAGS_RFO 0x04000000
#define MASK_FLAGS_RFU 0x08000000
#define MASK_FLAGS_RFL 0x10000000
#define MASK_FLAGS_TFO 0x40000000
#define MASK_FLAGS_TFL 0x80000000

#define MASK_RXFIFOCON_FLUSH 0x1
#define MASK_RXFIFOCON_ENI 0x2
#define MASK_RXFIFOCON_OUTW 0xC0

#define MASK_FLAGSENABLE_RFLE 0x10000000
#define MASK_FLAGSENABLE_TFLE 0x80000000

#define ASCLIN_R_MAX 27
#define ASCLIN_RX_BUFFER 8192

#define TYPE_TRICORE_ASCLIN "tricore_asclin"
#define TRICORE_ASCLIN(obj) \
    OBJECT_CHECK(TriCoreASCLINState, (obj), TYPE_TRICORE_ASCLIN)

struct TriCoreASCLINState {
    SysBusDevice parent_obj;
    MemoryRegion iomem;
    CharBackend chr;
    qemu_irq RXSR;
    qemu_irq TXSR;
    qemu_irq EXSR;
    guint watch_tag;
    uint32_t regs[ASCLIN_R_MAX];
    uint32_t txbuf;
    /* ToDo Extend buffer according to specification. */
    uint8_t rxbuf[ASCLIN_RX_BUFFER];
    uint32_t rxbufwriteidx;
    uint32_t rxbufreadidx;
    ptimer_state *ptimer;
    QEMUBH *bh;
};
typedef struct TriCoreASCLINState TriCoreASCLINState;


#endif
