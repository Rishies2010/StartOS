#ifndef APIC_H
#define APIC_H

#include <stdint.h>
#include <stdbool.h>

#define APIC_BASE_MSR           0x1B
#define APIC_BASE_BSP           (1 << 8)
#define APIC_BASE_ENABLE        (1 << 11)
#define APIC_BASE_X2APIC        (1 << 10)

#define APIC_ID                 0x020
#define APIC_VERSION            0x030
#define APIC_TPR                0x080
#define APIC_APR                0x090
#define APIC_PPR                0x0A0
#define APIC_EOI                0x0B0
#define APIC_RRD                0x0C0
#define APIC_LDR                0x0D0
#define APIC_DFR                0x0E0
#define APIC_SPURIOUS           0x0F0
#define APIC_ESR                0x280
#define APIC_ICRL               0x300
#define APIC_ICRH               0x310
#define APIC_LVT_TIMER          0x320
#define APIC_LVT_THERMAL        0x330
#define APIC_LVT_PERFCNT        0x340
#define APIC_LVT_LINT0          0x350
#define APIC_LVT_LINT1          0x360
#define APIC_LVT_ERROR          0x370
#define APIC_TIMER_ICR          0x380
#define APIC_TIMER_CCR          0x390
#define APIC_TIMER_DCR          0x3E0

#define APIC_ICR_DELIVERY_FIXED         (0 << 8)
#define APIC_ICR_DELIVERY_LOWEST        (1 << 8)
#define APIC_ICR_DELIVERY_SMI           (2 << 8)
#define APIC_ICR_DELIVERY_NMI           (4 << 8)
#define APIC_ICR_DELIVERY_INIT          (5 << 8)
#define APIC_ICR_DELIVERY_STARTUP       (6 << 8)

#define APIC_ICR_DEST_PHYSICAL          (0 << 11)
#define APIC_ICR_DEST_LOGICAL           (1 << 11)

#define APIC_ICR_DELIVERY_PENDING       (1 << 12)
#define APIC_ICR_LEVEL_ASSERT           (1 << 14)
#define APIC_ICR_LEVEL_DEASSERT         (0 << 14)
#define APIC_ICR_TRIGGER_EDGE           (0 << 15)
#define APIC_ICR_TRIGGER_LEVEL          (1 << 15)

#define APIC_ICR_DEST_SELF              (1 << 18)
#define APIC_ICR_DEST_ALL               (2 << 18)
#define APIC_ICR_DEST_ALL_BUT_SELF      (3 << 18)

#define APIC_LVT_MASKED                 (1 << 16)
#define APIC_LVT_TIMER_PERIODIC         (1 << 17)
#define APIC_LVT_TIMER_TSC_DEADLINE     (2 << 17)

#define APIC_SPURIOUS_ENABLE            (1 << 8)

#define APIC_TIMER_DIV_1                0xB
#define APIC_TIMER_DIV_2                0x0
#define APIC_TIMER_DIV_4                0x1
#define APIC_TIMER_DIV_8                0x2
#define APIC_TIMER_DIV_16               0x3
#define APIC_TIMER_DIV_32               0x8
#define APIC_TIMER_DIV_64               0x9
#define APIC_TIMER_DIV_128              0xA

bool init_apic(void);
void apic_enable(void);
void apic_disable(void);
bool apic_is_enabled(void);
uint32_t apic_get_id(void);
void apic_send_eoi(void);
void apic_send_ipi(uint32_t dest, uint32_t flags, uint32_t vector);
void apic_send_init_ipi(uint32_t dest);
void apic_send_startup_ipi(uint32_t dest, uint32_t vector);
void apic_timer_init(uint32_t frequency);
void apic_timer_stop(void);
uint32_t apic_timer_get_ticks(void);
void apic_mask_timer(void);
void apic_unmask_timer(void);

#endif