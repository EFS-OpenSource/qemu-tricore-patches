/*
 * QEMU model of the TriCore ASCLIN UART controller.
 *
 * Copyright (c) 2017 David Brenken
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 *
 */
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qapi/error.h"
#include "trace.h"
#include "hw/sysbus.h"
#include "hw/registerfields.h"
#include "chardev/char-fe.h"
#include "chardev/char-serial.h"
#include "qemu/error-report.h"
#include "migration/vmstate.h"
#include "hw/char/tricore_asclin.h"
#include "hw/qdev-properties-system.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

REG32(STATE, 4)
FIELD(STATE, TXFULL, 0, 1)
FIELD(STATE, RXFULL, 1, 1)

#define ASCLINUART_TIMERVAL 7000000

enum {
    CLC = 0,
    IOCR,
    ID,
    TXFIFOCON,
    RXFIFOCON,
    BITCON,
    FRAMECON,
    DATCON,
    BRG,
    BRD,
    LINCON,
    LINBTIMER,
    LINHTIMER,
    FLAGS,
    FLAGSSET,
    FLAGSCLEAR,
    FLAGSENABLE,
    TXDATA,
    RXDATA,
    CSR,
    RXDATAD,
};

static void uart_update_irq(TriCoreASCLINState *s)
{
    int rfe = (s->regs[FLAGSENABLE] & MASK_FLAGSENABLE_RFLE);
    int tfe = (s->regs[FLAGSENABLE] & MASK_FLAGSENABLE_TFLE);
    int rfl = (s->regs[FLAGS] & MASK_FLAGS_RFL);
    int tfl = (s->regs[FLAGS] & MASK_FLAGS_TFL);

    if (rfe && rfl) {
        qemu_irq_raise(s->RXSR);
    } else {
        qemu_irq_lower(s->RXSR);
    }
    if (tfe && tfl) {
        qemu_irq_raise(s->TXSR);
    } else {
        qemu_irq_lower(s->TXSR);
    }
}

static void uart_update_flags(TriCoreASCLINState *s)
{
    /* when there is data to receive, set the RFL flag */
    if (s->rxbufreadidx != s->rxbufwriteidx) {
        qatomic_or(&s->regs[FLAGS], MASK_FLAGS_RFL);
    } else {
        qatomic_and(&s->regs[FLAGS], ~MASK_FLAGS_RFL);
    }

    /* always set TFL, buffer is always empty */
    qatomic_or(&s->regs[FLAGS], MASK_FLAGS_TFL);

    uart_update_irq(s);
}

static void asclin_buffer_reset(TriCoreASCLINState *s)
{
    memset(s->rxbuf, 0x00, ASCLIN_RX_BUFFER);
    s->rxbufreadidx = 0;
    s->rxbufwriteidx = 0;

    uart_update_flags(s);
}

static uint32_t asclin_buffer_used(TriCoreASCLINState *s)
{
    uint32_t ret = ((s->rxbufwriteidx + ASCLIN_RX_BUFFER) - s->rxbufreadidx)
        % ASCLIN_RX_BUFFER;

    return ret;
}

static uint32_t asclin_buffer_free(TriCoreASCLINState *s)
{
    return asclin_buffer_used(s) - (ASCLIN_RX_BUFFER - 1);
}


/* Try to send tx data, and arrange to be called back later if
 * we can't (ie the char backend is busy/blocking).
 */
static gboolean uart_transmit(void *do_not_use, GIOCondition cond, void *opaque)
{
    TriCoreASCLINState *s = TRICORE_ASCLIN(opaque);
    int ret;

    s->watch_tag = 0;

    /* qemu_log("QEMU->Host 0x%x\n", (int) s->txbuf); */
    ret = qemu_chr_fe_write_all(&s->chr, (uint8_t *) (&s->txbuf), 1);
    if (ret <= 0) {
        s->watch_tag = qemu_chr_fe_add_watch(&s->chr, G_IO_OUT | G_IO_HUP,
                uart_transmit, s);
        if (!s->watch_tag) {
            /* Most common reason to be here is "no chardev backend":
             * just insta-drain the buffer, so the serial output
             * goes into a void, rather than blocking the guest.
             */
            goto buffer_drained;
        }
        /* Transmit pending */
        return FALSE;
    }

    buffer_drained:

    /* Character successfully sent */
    qatomic_or(&s->regs[FLAGS], MASK_FLAGS_TC);

    uart_update_flags(s);

    return FALSE;
}

static gboolean uart_transmit_block(void *do_not_use, GIOCondition cond,
    void *opaque, void *buf, uint32_t length)
{
    TriCoreASCLINState *s = TRICORE_ASCLIN(opaque);
    int ret;

    s->watch_tag = 0;

    ret = qemu_chr_fe_write_all(&s->chr, (uint8_t *) buf, length);
    if (ret <= 0) {
        s->watch_tag = qemu_chr_fe_add_watch(&s->chr, G_IO_OUT | G_IO_HUP,
                uart_transmit, s);
        if (!s->watch_tag) {
            /*
             * Most common reason to be here is "no chardev backend":
             * just insta-drain the buffer, so the serial output
             * goes into a void, rather than blocking the guest.
             */
            goto buffer_drained;
        }
        /* Transmit pending */
        return FALSE;
    }

    buffer_drained:

    /* Character successfully sent */
    qatomic_or(&s->regs[FLAGS], MASK_FLAGS_TC);

    uart_update_flags(s);

    return FALSE;
}

static uint64_t uart_read(void *opaque, hwaddr offset, unsigned size)
{
    hwaddr reg_addr;

    reg_addr = offset >> 2;

    TriCoreASCLINState *s = opaque;
    uint32_t r = 0;

    switch (reg_addr) {

    case CLC:
    case IOCR:
    case ID:
    case TXFIFOCON:
    case RXFIFOCON:
    case BITCON:
    case FRAMECON:
    case DATCON:
    case BRG:
    case BRD:
    case LINCON:
    case LINBTIMER:
    case LINHTIMER:
    case FLAGSENABLE:
    case FLAGS:
        r = (s->regs[reg_addr]) >> ((offset & 0x3) * 0x8);
        break;
    case FLAGSSET:
    case FLAGSCLEAR:
        r = 0x0;
        break;

    case TXDATA:
        r = 0x0;
        qemu_log("uart_read: TXDATA offset 0x%x, r: 0x%x\n", (int) offset,
                (int) r);
        break;
    case RXDATA:
        /* get last received byte */
        r = s->rxbuf[s->rxbufreadidx];

        /* fill in next byte if there is more */
        if (s->rxbufreadidx != s->rxbufwriteidx) {
            s->rxbufreadidx++;
            s->rxbufreadidx = (s->rxbufreadidx) % ASCLIN_RX_BUFFER;
        } else {
            qemu_log("uart_read: RXDATA was read although buffer is empty\n");
        }

        /* qemu_log("Host->QEMU 0x%x\n", (int) r); */
        break;
    case CSR:
    {
        uint32_t csr = s->regs[reg_addr];
        csr |= (csr & 0x0F) ? (1 << 31) : 0;
        r = csr >> ((offset & 0x3) * 0x8);
        break;
    }
    case RXDATAD:
        /* get last received byte */
        r = s->rxbuf[s->rxbufreadidx];
        break;
    default:
        error_report("asclin_uart: read access to unknown register 0x"
        HWADDR_FMT_plx, reg_addr << 2);
        break;
    }

    uart_update_flags(s);
    return r;
}

static void uart_write(void *opaque, hwaddr offset, uint64_t value,
        unsigned size)
{
    hwaddr reg_addr;
    TriCoreASCLINState *s = opaque;

    /* Since we can only write a byte, we have to calculate and shift
     some values. */
    reg_addr = offset >> 2;
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

    switch (reg_addr) {

    case CLC:
    case IOCR:
    case ID:
    case TXFIFOCON:
        s->regs[reg_addr] = value;
        break;
    case RXFIFOCON:
        s->regs[reg_addr] = value;

        /* Flush rx buffer. */
        if ((value & MASK_RXFIFOCON_FLUSH) >> 0) {
            asclin_buffer_reset(s);
        }
        /* If RXFIFO is enabled, the character backend device is accepting
         input. */
        if ((value & MASK_RXFIFOCON_ENI) >> 1) {
            qemu_chr_fe_accept_input(&s->chr);
        }
        break;
    case BITCON:
        /* write one to clear bits */
        s->regs[reg_addr] &= ~(value & (STAT_RX_EVT | STAT_TX_EVT));
        break;
    case FRAMECON:
    case DATCON:
    case BRG:
    case BRD:
    case LINCON:
    case LINBTIMER:
        break;
    case LINHTIMER:
        break;
    case FLAGS:
        s->regs[FLAGS] = value;
        break;
    case FLAGSSET:
        qatomic_or(&s->regs[FLAGS], value);
        break;
    case FLAGSCLEAR:
        qatomic_and(&s->regs[FLAGS], ~value);
        break;
    case FLAGSENABLE:
        s->regs[reg_addr] = value;
        break;
    case TXDATA:
        s->txbuf = value;
        uart_transmit(NULL, G_IO_OUT, s);
        break;
    case RXDATA:
    case CSR:
        s->regs[reg_addr] = value;
        break;
    case RXDATAD:
        break;

    /* special interface: block TXDATA length */
    case 0x60 / 4:
        s->regs[reg_addr] = value;
        break;

    /* special interface: block TXDATA buffer */
    case 0x64 / 4:
    {
        uint32_t xfer_len = s->regs[0x60 / 4];
        void *buf = malloc(xfer_len);

        cpu_physical_memory_read(value, buf, xfer_len);
        uart_transmit_block(NULL, G_IO_OUT, s, buf, xfer_len);

        free(buf);
        break;
    }
    default:
        error_report("asclin_uart: write access to unknown register 0x"
        HWADDR_FMT_plx, reg_addr << 2);
        break;
    }
    uart_update_irq(s);
}

static const MemoryRegionOps asclin_uart_mmio_ops = {
        .read = uart_read, .write = uart_write, .valid = {
                .min_access_size = 1, .max_access_size = 4, }, .endianness =
                DEVICE_NATIVE_ENDIAN, };

static void uart_rx(void *opaque, const uint8_t *buf, int size)
{
    TriCoreASCLINState *s = opaque;

    /* lock flags and fill buffer */
    while (size > 0) {
        s->rxbuf[s->rxbufwriteidx] = *buf;
        s->rxbufwriteidx++;
        s->rxbufwriteidx = s->rxbufwriteidx % ASCLIN_RX_BUFFER;

        if (asclin_buffer_free(s) == 0) {
            error_report(
                "asclin_uart: RX buffer overflowed, %d bytes dropped",
                size);
                break;
        }
        size--;
    }

    uart_update_flags(s);
}

static int uart_can_rx(void *opaque)
{
    TriCoreASCLINState *s = TRICORE_ASCLIN(opaque);

    /* We can take a char if RX is enabled and the buffer is not full */
    if ((s->regs[RXFIFOCON] & MASK_RXFIFOCON_ENI) &&
        asclin_buffer_free(s) > 0) {
        return 1;
    }
    return 0;
}

static void uart_event(void *opaque, QEMUChrEvent event)
{
}

static void asclin_uart_reset(DeviceState *d)
{
    TriCoreASCLINState *s = TRICORE_ASCLIN(d);
    int i;

    for (i = 0; i < ASCLIN_R_MAX; i++) {
        s->regs[i] = 0;
    }
}

static void asclin_uart_realize(DeviceState *dev, Error **errp)
{
    TriCoreASCLINState *s = TRICORE_ASCLIN(dev);

    qemu_chr_fe_set_handlers(&s->chr, uart_can_rx, uart_rx, uart_event, NULL, s,
            NULL, true);
}
static void asclin_uart_update_parameters(TriCoreASCLINState *s);

static void asclin_uart_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    TriCoreASCLINState *s = TRICORE_ASCLIN(obj);

    memory_region_init_io(&s->iomem, obj, &asclin_uart_mmio_ops, s, "uart",
            0xFF);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->RXSR);
    sysbus_init_irq(sbd, &s->TXSR);
    sysbus_init_irq(sbd, &s->EXSR);
    s->rxbufreadidx = 0;
    s->rxbufwriteidx = 0;

    asclin_uart_update_parameters(s);
}

static void asclin_uart_update_parameters(TriCoreASCLINState *s)
{
    QEMUSerialSetParams ssp;

    ssp.speed = 921600;
    ssp.data_bits = 8;
    ssp.parity = 'N';
    ssp.stop_bits = 1;
    qemu_chr_fe_ioctl(&s->chr, CHR_IOCTL_SERIAL_SET_PARAMS, &ssp);
}

static int asclin_uart_post_load(void *opaque, int version_id)
{
    TriCoreASCLINState *s = TRICORE_ASCLIN(opaque);

    /* If we have a pending character, arrange to resend it. */
    if ((s->regs[FLAGS] & MASK_FLAGS_TFL)) {
        s->watch_tag = qemu_chr_fe_add_watch(&s->chr, G_IO_OUT | G_IO_HUP,
                uart_transmit, s);
    }
    asclin_uart_update_parameters(s);
    return 0;
}

static const VMStateDescription vmstate_asclin_uart = {
                .name = "asclin-uart",
                .version_id = 1,
                .minimum_version_id = 1,
                .fields =
                    (VMStateField[]) {
                      VMSTATE_UINT32_ARRAY(regs, TriCoreASCLINState, ASCLIN_R_MAX),
                      VMSTATE_END_OF_LIST() }, .post_load =
                       asclin_uart_post_load };

static Property asclin_uart_properties[] = {
        DEFINE_PROP_CHR("chardev", TriCoreASCLINState, chr),
        DEFINE_PROP_END_OF_LIST(), };

static void asclin_uart_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = asclin_uart_realize;
    dc->reset = asclin_uart_reset;
    dc->vmsd = &vmstate_asclin_uart;
    device_class_set_props(dc, asclin_uart_properties);
}

static const TypeInfo asclin_uart_info = {
        .name = TYPE_TRICORE_ASCLIN, .parent = TYPE_SYS_BUS_DEVICE,
        .instance_size = sizeof(TriCoreASCLINState), .instance_init =
                asclin_uart_init, .class_init = asclin_uart_class_init, };

static void asclin_uart_register_types(void)
{
    type_register_static(&asclin_uart_info);
}

type_init(asclin_uart_register_types)
