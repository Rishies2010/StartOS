#include "smp.h"
#include "../libk/debug/log.h"
#include "../drv/local_apic.h"
#include "../drv/rtc.h"

#define g_activeCpuCount (*(volatile uint32_t*)0x00006008)

// ------------------------------------------------------------------------------------------------
void init_smp()
{
    log("[SMP] Waking up all CPUs.", 1, 0);

    g_activeCpuCount = 1;
    int localId = LocalApicGetId();
    for (int i = 0; i < g_acpiCpuCount; ++i)
    {
        int apicId = g_acpiCpuIds[i];
        if (apicId != localId)
        {
            LocalApicSendInit(apicId);
        }
    }
    sleep(10);
    for (int i = 0; i < g_acpiCpuCount; ++i)
    {
        int apicId = g_acpiCpuIds[i];
        if (apicId != localId)
        {
            LocalApicSendStartup(apicId, 0x8);
        }
    }
    sleep(10);
    while (g_activeCpuCount != g_acpiCpuCount)
    {
        log("[SMP] Waiting : %d.", 1, 0, g_activeCpuCount);
        sleep(10);
    }

    log("[SMP] All CPUs activated.", 4, 0);
}