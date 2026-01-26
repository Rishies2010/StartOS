#include "acpi.h"
#include "../../drv/ioapic.h"
#include "../../drv/local_apic.h"
#include "../../libk/string.h"
#include "../../libk/debug/log.h"
#include "stdint.h"
#include "stdbool.h"
#include "../../drv/vga.h"

int g_acpiCpuCount;
uint8_t g_acpiCpuIds[MAX_CPU_COUNT];

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
    uint8_t s4BiosReq;
    uint8_t pStateCnt;
    uint32_t pm1aEventBlk;
    uint32_t pm1bEventBlk;
    uint32_t pm1aControlBlk;
    uint32_t pm1bControlBlk;
    uint32_t pm2ControlBlk;
    uint32_t pmTimerBlk;
    uint32_t gpe0Blk;
    uint32_t gpe1Blk;
    uint8_t pm1EventLength;
    uint8_t pm1ControlLength;
    uint8_t pm2ControlLength;
    uint8_t pmTimerLength;
    uint8_t gpe0BlkLength;
    uint8_t gpe1BlkLength;
    uint8_t gpe1Base;
    uint8_t cStateControl;
    uint16_t worstC2Latency;
    uint16_t worstC3Latency;
    uint16_t flushSize;
    uint16_t flushStride;
    uint8_t dutyOffset;
    uint8_t dutyWidth;
    uint8_t dayAlrm;
    uint8_t monAlrm;
    uint8_t century;
    uint16_t bootArchFlags;
    uint8_t reserved2;
    uint32_t flags;
    uint8_t resetReg[12];
    uint8_t resetValue;
    uint16_t armBootArchFlags;
    uint8_t fadtMinorVersion;
    uint64_t xFirmwareControl;
    uint64_t xDsdt;
    uint8_t xPm1aEventBlk[12];
    uint8_t xPm1bEventBlk[12];
    uint8_t xPm1aControlBlk[12];
    uint8_t xPm1bControlBlk[12];
    uint8_t xPm2ControlBlk[12];
    uint8_t xPmTimerBlk[12];
    uint8_t xGpe0Blk[12];
    uint8_t xGpe1Blk[12];
    uint8_t sleepControlReg[12];
    uint8_t sleepStatusReg[12];
    uint64_t hypervisorVendorId;
} __attribute__((__packed__)) AcpiFadt;

static AcpiFadt *s_fadt;

typedef struct AcpiMadt
{
    AcpiHeader header;
    uint32_t localApicAddr;
    uint32_t flags;
} __attribute__((__packed__)) AcpiMadt;

typedef struct ApicHeader
{
    uint8_t type;
    uint8_t length;
} __attribute__((__packed__)) ApicHeader;

#define APIC_TYPE_LOCAL_APIC 0
#define APIC_TYPE_IO_APIC 1
#define APIC_TYPE_INTERRUPT_OVERRIDE 2

typedef struct ApicLocalApic
{
    ApicHeader header;
    uint8_t acpiProcessorId;
    uint8_t apicId;
    uint32_t flags;
} __attribute__((__packed__)) ApicLocalApic;

typedef struct ApicIoApic
{
    ApicHeader header;
    uint8_t ioApicId;
    uint8_t reserved;
    uint32_t ioApicAddress;
    uint32_t globalSystemInterruptBase;
} __attribute__((__packed__)) ApicIoApic;

typedef struct ApicInterruptOverride
{
    ApicHeader header;
    uint8_t bus;
    uint8_t source;
    uint32_t interrupt;
    uint16_t flags;
} __attribute__((__packed__)) ApicInterruptOverride;

static AcpiMadt *s_madt;
static uint16_t s_slp_typa = 0;
static uint16_t s_slp_typb = 0;
static bool s_s5_found = false;

static bool AcpiFindS5(uint8_t *dsdt_addr, uint32_t dsdt_length)
{
    uint8_t *dsdt = dsdt_addr;
    uint32_t *signature = (uint32_t *)dsdt;

    if (*signature != 0x54445344)
        return false;

    uint8_t *end = dsdt + dsdt_length;
    uint8_t *ptr = dsdt + sizeof(AcpiHeader);

    while (ptr < end - 4)
    {
        if (ptr[0] == '_' && ptr[1] == 'S' && ptr[2] == '5' && ptr[3] == '_')
        {
            ptr += 4;

            if (*ptr == 0x12)
            {
                ptr++;
                uint8_t pkg_length = *ptr++;
                if (pkg_length >= 4)
                {
                    ptr++;
                    if (*ptr == 0x0A)
                    {
                        ptr++;
                        s_slp_typa = *ptr++;
                    }
                    if (*ptr == 0x0A)
                    {
                        ptr++;
                        s_slp_typb = *ptr++;
                    }
                    s_s5_found = true;
                    return true;
                }
            }
        }
        ptr++;
    }
    return false;
}

static void AcpiParseFacp(AcpiFadt *facp)
{
    s_fadt = facp;

    log("PM1A Control Block = 0x%08x", 1, 0, facp->pm1aControlBlk);
    log("PM1B Control Block = 0x%08x", 1, 0, facp->pm1bControlBlk);
    log("Reset Register Address = 0x%02x%02x%02x%02x", 1, 0,
        facp->resetReg[8], facp->resetReg[9], facp->resetReg[10], facp->resetReg[11]);
    log("Reset Value = 0x%02x", 1, 0, facp->resetValue);
    log("Flags = 0x%08x", 1, 0, facp->flags);
    log("DSDT = 0x%08x", 1, 0, facp->dsdt);

    if (facp->dsdt)
    {
        AcpiHeader *dsdt_header = (AcpiHeader *)(uintptr_t)facp->dsdt;
        AcpiFindS5((uint8_t *)dsdt_header, dsdt_header->length);
        if (s_s5_found)
        {
            log("Found _S5_: SLP_TYPa=0x%02x SLP_TYPb=0x%02x", 1, 0, s_slp_typa, s_slp_typb);
        }
    }

    if (facp->smiCommandPort)
    {
        log("Enabling ACPI", 1, 0);
        asm volatile("outb %0, %1" : : "a"((uint8_t)facp->acpiEnable), "Nd"((uint16_t)facp->smiCommandPort));

        uint16_t status;
        do
        {
            asm volatile("inw %1, %0" : "=a"(status) : "Nd"((uint16_t)facp->pm1aControlBlk));
        } while (!(status & 0x0001));
    }
    else
    {
        log("ACPI already enabled", 2, 1);
    }
}

void AcpiShutdown()
{
    log("Shutting down...", 3, 1);
    if (!s_fadt || !s_s5_found)
    {
        asm volatile("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
        asm volatile("cli; hlt" : : : "memory");
        return;
    }

    uint16_t pm1a_cnt = s_fadt->pm1aControlBlk;
    uint16_t pm1b_cnt = s_fadt->pm1bControlBlk;

    if (pm1a_cnt == 0)
    {
        asm volatile("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
        asm volatile("cli; hlt" : : : "memory");
        return;
    }

    uint16_t pm1a_status;
    asm volatile("inw %1, %0" : "=a"(pm1a_status) : "Nd"(pm1a_cnt));
    pm1a_status &= ~(0x7 << 10);
    pm1a_status |= (s_slp_typa << 10) | (1 << 13);

    asm volatile("cli" : : : "memory");
    asm volatile("outw %0, %1" : : "a"(pm1a_status), "Nd"(pm1a_cnt));

    if (pm1b_cnt != 0)
    {
        uint16_t pm1b_status;
        asm volatile("inw %1, %0" : "=a"(pm1b_status) : "Nd"(pm1b_cnt));
        pm1b_status &= ~(0x7 << 10);
        pm1b_status |= (s_slp_typb << 10) | (1 << 13);
        asm volatile("outw %0, %1" : : "a"(pm1b_status), "Nd"(pm1b_cnt));
    }

    for (volatile int i = 0; i < 1000000; i++)
        ;

    asm volatile("outb %0, %1" : : "a"((uint8_t)0xFE), "Nd"((uint16_t)0x64));
    log("Failed to shutdown. Aborting.", 3, 1);
}

static void AcpiParseApic(AcpiMadt *madt)
{
    s_madt = madt;

    log("Local APIC Address = 0x%08x", 1, 0, madt->localApicAddr);
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

            log("Found CPU: %d %d %x", 1, 0, s->acpiProcessorId, s->apicId, s->flags);
            if (g_acpiCpuCount < MAX_CPU_COUNT)
            {
                g_acpiCpuIds[g_acpiCpuCount] = s->apicId;
                ++g_acpiCpuCount;
            }
        }
        else if (type == APIC_TYPE_IO_APIC)
        {
            ApicIoApic *s = (ApicIoApic *)p;

            log("Found I/O APIC: %d 0x%08x %d", 1, 0, s->ioApicId, s->ioApicAddress, s->globalSystemInterruptBase);
            g_ioApicAddr = (uint8_t *)(uintptr_t)s->ioApicAddress;
        }
        else if (type == APIC_TYPE_INTERRUPT_OVERRIDE)
        {
            ApicInterruptOverride *s = (ApicInterruptOverride *)p;

            log("Found Interrupt Override: %d %d %d 0x%04x", 1, 0, s->bus, s->source, s->interrupt, s->flags);
        }
        else
        {
            log("Unknown APIC structure %d", 2, 0, type);
        }

        p += length;
    }
}

static void AcpiParseDT(AcpiHeader *header)
{
    uint32_t signature = header->signature;

    char sigStr[5];
    memcpy(sigStr, &signature, 4);
    sigStr[4] = 0;
    log("%s 0x%x", 1, 0, sigStr, signature);

    if (signature == 0x50434146)
    {
        AcpiParseFacp((AcpiFadt *)header);
    }
    else if (signature == 0x43495041)
    {
        AcpiParseApic((AcpiMadt *)header);
    }
}

static void AcpiParseRsdt(AcpiHeader *rsdt)
{
    uint32_t *p = (uint32_t *)(rsdt + 1);
    uint32_t *end = (uint32_t *)((uint8_t *)rsdt + rsdt->length);

    while (p < end)
    {
        uint32_t address = *p++;
        AcpiParseDT((AcpiHeader *)(uintptr_t)address);
    }
}

static void AcpiParseXsdt(AcpiHeader *xsdt)
{
    uint64_t *p = (uint64_t *)(xsdt + 1);
    uint64_t *end = (uint64_t *)((uint8_t *)xsdt + xsdt->length);

    while (p < end)
    {
        uint64_t address = *p++;
        AcpiParseDT((AcpiHeader *)(uintptr_t)address);
    }
}

static bool AcpiParseRsdp(uint8_t *p)
{
    log("RSDP found", 1, 0);
    uint8_t sum = 0;
    for (int i = 0; i < 20; ++i)
    {
        sum += p[i];
    }

    if (sum)
    {
        log("Checksum failed", 3, 1);
        return false;
    }

    char oem[7];
    memcpy(oem, p + 9, 6);
    oem[6] = '\0';
    log("OEM = %s", 1, 0, oem);

    uint8_t revision = p[15];
    if (revision == 0)
    {
        log("Version 1", 1, 0);

        uint32_t rsdtAddr = *(uint32_t *)(p + 16);
        AcpiParseRsdt((AcpiHeader *)(uintptr_t)rsdtAddr);
    }
    else if (revision == 2)
    {
        log("Version 2", 1, 0);

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
        log("Unsupported ACPI version %d", 3, 1, revision);
    }

    return true;
}

void AcpiInit()
{
    uint8_t *p = (uint8_t *)0x000e0000;
    uint8_t *end = (uint8_t *)0x000fffff;

    while (p < end)
    {
        uint64_t signature = *(uint64_t *)p;

        if (signature == 0x2052545020445352)
        {
            if (AcpiParseRsdp(p))
            {
                break;
            }
        }

        p += 16;
    }
}

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
