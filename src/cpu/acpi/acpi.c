// ------------------------------------------------------------------------------------------------
// acpi/acpi.c
// ------------------------------------------------------------------------------------------------

#include "acpi.h"
#include "../../drv/ioapic.h"
#include "../../drv/local_apic.h"
#include "../../libk/string.h"
#include "../../libk/debug/log.h"
#include "stdint.h"
#include "stdbool.h"
#include "../../drv/vga.h"

// ------------------------------------------------------------------------------------------------
// Globals
int g_acpiCpuCount;
uint8_t g_acpiCpuIds[MAX_CPU_COUNT];

// ------------------------------------------------------------------------------------------------
typedef struct AcpiHeader
{
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    uint8_t oem[6];
    uint8_t oemTableId[8];
    uint32_t oemRevision;
    uint32_t creatorId;
    uint32_t creatorRevision;
} __attribute__((__packed__)) AcpiHeader;

// ------------------------------------------------------------------------------------------------
typedef struct AcpiFadt
{
    AcpiHeader header;
    uint32_t firmwareControl;
    uint32_t dsdt;
    uint8_t reserved;
    uint8_t preferredPMProfile;
    uint16_t sciInterrupt;
    uint32_t smiCommandPort;
    uint8_t acpiEnable;
    uint8_t acpiDisable;
    // TODO - fill in rest of data
} __attribute__((__packed__)) AcpiFadt;

// ------------------------------------------------------------------------------------------------
typedef struct AcpiMadt
{
    AcpiHeader header;
    uint32_t localApicAddr;
    uint32_t flags;
} __attribute__((__packed__)) AcpiMadt;

// ------------------------------------------------------------------------------------------------
typedef struct ApicHeader
{
    uint8_t type;
    uint8_t length;
} __attribute__((__packed__)) ApicHeader;

// APIC structure types
#define APIC_TYPE_LOCAL_APIC            0
#define APIC_TYPE_IO_APIC               1
#define APIC_TYPE_INTERRUPT_OVERRIDE    2

// ------------------------------------------------------------------------------------------------
typedef struct ApicLocalApic
{
    ApicHeader header;
    uint8_t acpiProcessorId;
    uint8_t apicId;
    uint32_t flags;
} __attribute__((__packed__)) ApicLocalApic;

// ------------------------------------------------------------------------------------------------
typedef struct ApicIoApic
{
    ApicHeader header;
    uint8_t ioApicId;
    uint8_t reserved;
    uint32_t ioApicAddress;
    uint32_t globalSystemInterruptBase;
} __attribute__((__packed__)) ApicIoApic;

// ------------------------------------------------------------------------------------------------
typedef struct ApicInterruptOverride
{
    ApicHeader header;
    uint8_t bus;
    uint8_t source;
    uint32_t interrupt;
    uint16_t flags;
} __attribute__((__packed__)) ApicInterruptOverride;

// ------------------------------------------------------------------------------------------------
static AcpiMadt *s_madt;

// ------------------------------------------------------------------------------------------------
static void AcpiParseFacp(AcpiFadt *facp)
{
    if (facp->smiCommandPort)
    {
        //log("Enabling ACPI", 1, 0);
        //IoWrite8(facp->smiCommandPort, facp->acpiEnable);

        // TODO - wait for SCI_EN bit
    }
    else
    {
        log("[AcpiParseFacp] ACPI already enabled.", 1, 0);
        log("[AcpiParseFacp] WIP Function Called.", 2, 1);

    }
}

// ------------------------------------------------------------------------------------------------
static void AcpiParseApic(AcpiMadt *madt)
{
    s_madt = madt;

    log("[AcpiParseApic] Local APIC Address = 0x%08x", 1, 0, madt->localApicAddr);
    g_localApicAddr = (uint8_t *)(uintptr_t)madt->localApicAddr;

    uint8_t *p = (uint8_t *)(madt + 1);
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    while (p < end)
    {
        ApicHeader *header = (ApicHeader *)p;
        uint8_t type = header->type;
        uint8_t length = header->length;

        if (type == APIC_TYPE_LOCAL_APIC)
        {
            ApicLocalApic *s = (ApicLocalApic *)p;

            log("[AcpiParseApic] Found CPU: %d %d %x", 1, 0, s->acpiProcessorId, s->apicId, s->flags);
            if (g_acpiCpuCount < MAX_CPU_COUNT)
            {
                g_acpiCpuIds[g_acpiCpuCount] = s->apicId;
                ++g_acpiCpuCount;
            }
        }
        else if (type == APIC_TYPE_IO_APIC)
        {
            ApicIoApic *s = (ApicIoApic *)p;

            log("[AcpiParseApic] Found I/O APIC: %d 0x%08x %d", 1, 0, s->ioApicId, s->ioApicAddress, s->globalSystemInterruptBase);
            g_ioApicAddr = (uint8_t *)(uintptr_t)s->ioApicAddress;
        }
        else if (type == APIC_TYPE_INTERRUPT_OVERRIDE)
        {
            ApicInterruptOverride *s = (ApicInterruptOverride *)p;

            log("[AcpiParseApic] Found Interrupt Override: %d %d %d 0x%04x", 1, 0, s->bus, s->source, s->interrupt, s->flags);
        }
        else
        {
            log("[AcpiParseApic] Unknown APIC structure %d", 2, 0, type);
        }

        p += length;
    }
}

// ------------------------------------------------------------------------------------------------
static void AcpiParseDT(AcpiHeader *header)
{
    uint32_t signature = header->signature;

    char sigStr[5];
    memcpy(sigStr, &signature, 4);
    sigStr[4] = 0;
    log("[AcpiParseDT] %s 0x%x", 1, 0, sigStr, signature);

    if (signature == 0x50434146)
    {
        AcpiParseFacp((AcpiFadt *)header);
    }
    else if (signature == 0x43495041)
    {
        AcpiParseApic((AcpiMadt *)header);
    }
}

// ------------------------------------------------------------------------------------------------
static void AcpiParseRsdt(AcpiHeader *rsdt)
{
    uint32_t *p = (uint32_t *)(rsdt + 1);
    uint32_t *end = (uint32_t *)((uint8_t*)rsdt + rsdt->length);

    while (p < end)
    {
        uint32_t address = *p++;
        AcpiParseDT((AcpiHeader *)(uintptr_t)address);
    }
}

// ------------------------------------------------------------------------------------------------
static void AcpiParseXsdt(AcpiHeader *xsdt)
{
    uint64_t *p = (uint64_t *)(xsdt + 1);
    uint64_t *end = (uint64_t *)((uint8_t*)xsdt + xsdt->length);

    while (p < end)
    {
        uint64_t address = *p++;
        AcpiParseDT((AcpiHeader *)(uintptr_t)address);
    }
}

// ------------------------------------------------------------------------------------------------
static bool AcpiParseRsdp(uint8_t *p)
{
    // Parse Root System Description Pointer
    log("[AcpiParseRsdp] RSDP found", 1, 0);

    // Verify checksum
    uint8_t sum = 0;
    for (int i = 0; i < 20; ++i)
    {
        sum += p[i];
    }

    if (sum)
    {
        log("[AcpiParseRsdp] Checksum failed", 3, 1);
        return false;
    }

    // Print OEM
    char oem[7];
    memcpy(oem, p + 9, 6);
    oem[6] = '\0';
    log("[AcpiParseRsdp] OEM = %s", 1, 0, oem);

    // Check version
    uint8_t revision = p[15];
    if (revision == 0)
    {
        log("[AcpiParseRsdp] Version 1", 1, 0);

        uint32_t rsdtAddr = *(uint32_t *)(p + 16);
        AcpiParseRsdt((AcpiHeader *)(uintptr_t)rsdtAddr);
    }
    else if (revision == 2)
    {
        log("[AcpiParseRsdp] Version 2", 1, 0);

        uint32_t rsdtAddr = *(uint32_t *)(p + 16);
        uint64_t xsdtAddr = *(uint64_t *)(p + 24);

        if (xsdtAddr)
        {
            AcpiParseXsdt((AcpiHeader *)(uintptr_t)xsdtAddr);
        }
        else
        {
            AcpiParseRsdt((AcpiHeader *)(uintptr_t)rsdtAddr);
        }
    }
    else
    {
        log("[AcpiParseRsdp] Unsupported ACPI version %d", 3, 1, revision);
    }

    return true;
}

// ------------------------------------------------------------------------------------------------
void AcpiInit()
{
    // TODO - Search Extended BIOS Area

    // Search main BIOS area below 1MB
    uint8_t *p = (uint8_t *)0x000e0000;
    uint8_t *end = (uint8_t *)0x000fffff;

    while (p < end)
    {
        uint64_t signature = *(uint64_t *)p;

        if (signature == 0x2052545020445352) // 'RSD PTR '
        {
            if (AcpiParseRsdp(p))
            {
                break;
            }
        }

        p += 16;
    }
}

// ------------------------------------------------------------------------------------------------
int AcpiRemapIrq(int irq)
{
    AcpiMadt *madt = s_madt;

    uint8_t *p = (uint8_t *)(madt + 1);
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    while (p < end)
    {
        ApicHeader *header = (ApicHeader *)p;
        uint8_t type = header->type;
        uint8_t length = header->length;

        if (type == APIC_TYPE_INTERRUPT_OVERRIDE)
        {
            ApicInterruptOverride *s = (ApicInterruptOverride *)p;

            if (s->source == irq)
            {
                return s->interrupt;
            }
        }

        p += length;
    }

    return irq;
}
