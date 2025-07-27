#include "smp.h"
#include "../libk/debug/log.h"
#include "../drv/local_apic.h"
#include "../libk/string.h"

#define TRAMPOLINE_ADDR 0x7000
#define STACK_SIZE 4096

volatile uint32_t g_activeCpuCount = 1;
static uint8_t ap_stacks[MAX_CPU_COUNT][STACK_SIZE] __attribute__((aligned(16)));

void init_smp() {
    log("[SMP] Bootstrap Processor ID: %d, Total CPUs: %d", 1, 0, LocalApicGetId(), g_acpiCpuCount);
    
    const uint8_t tramp[] = {
        0xFA,
        0xF4,
        0xEB, 0xFD
    };
    
    memcpy((void*)TRAMPOLINE_ADDR, tramp, sizeof(tramp));
    
    int bsp_id = LocalApicGetId();
    
    for (int i = 0; i < g_acpiCpuCount; i++) {
        if (g_acpiCpuIds[i] != bsp_id) {
            log("[SMP] Starting CPU %i.", 1, 0, i);
            LocalApicSendInit(g_acpiCpuIds[i]);
            for (volatile int j = 0; j < 1000000; j++);
            
            LocalApicSendStartup(g_acpiCpuIds[i], TRAMPOLINE_ADDR >> 12);
            for (volatile int j = 0; j < 100000; j++);
            
            LocalApicSendStartup(g_acpiCpuIds[i], TRAMPOLINE_ADDR >> 12);
            for (volatile int j = 0; j < 1000000; j++);
            log("[SMP] CPU %i started and halted.", 4, 0, i);
            break;
        }
    }
}