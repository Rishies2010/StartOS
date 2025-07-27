#include "smp.h"
#include "../libk/debug/log.h"
#include "../drv/local_apic.h"
#include "../libk/string.h"
#include "../libk/limine.h"
#include "gdt.h"
#include "idt.h"

#define STACK_SIZE 4096

static volatile struct limine_smp_request smp_request = {
    .id = LIMINE_SMP_REQUEST,
    .revision = 0
};

volatile uint32_t g_activeCpuCount = 1;
static uint8_t ap_stacks[MAX_CPU_COUNT][STACK_SIZE] __attribute__((aligned(16)));

void ap_entry(struct limine_smp_info *info) {
    init_gdt();
    init_idt();
    LocalApicInit(info->lapic_id);
    asm volatile("mov %0, %%rsp" : : "r" (ap_stacks[info->processor_id] + STACK_SIZE) : "memory");
    __atomic_add_fetch(&g_activeCpuCount, 1, __ATOMIC_SEQ_CST);
    asm volatile("sti");
    while(1);
}

void init_smp() {
    struct limine_smp_response *smp = smp_request.response;
    if (smp == NULL || smp->cpu_count < 1) {
        log("[SMP] Limine SMP not available, falling back to single CPU", 2, 0);
        return;
    }
    log("[SMP] Bootstrap Processor ID: %d, Total CPUs: %d", 1, 0,
        smp->bsp_lapic_id, smp->cpu_count);
    LocalApicInit(smp->bsp_lapic_id);
    for (size_t i = 0; i < smp->cpu_count; i++) {
        if (smp->cpus[i]->lapic_id != smp->bsp_lapic_id) {
            log("[SMP] Starting CPU %lu (LAPIC ID %d)", 1, 0,
                i, smp->cpus[i]->lapic_id);
            smp->cpus[i]->goto_address = ap_entry;
        }
    }
    while (g_activeCpuCount < smp->cpu_count) {
        asm volatile("pause");
    }
    log("[SMP] All %d CPUs online", 4, 0, smp->cpu_count);
}