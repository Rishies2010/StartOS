// ------------------------------------------------------------------------------------------------
// acpi/acpi.h
// ------------------------------------------------------------------------------------------------

#include "stdint.h"
#include "stdbool.h"

#define MAX_CPU_COUNT 16

extern int g_acpiCpuCount;
extern uint8_t g_acpiCpuIds[MAX_CPU_COUNT];

void AcpiInit();
int AcpiRemapIrq(int irq);
void AcpiShutdown();