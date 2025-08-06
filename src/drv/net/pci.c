#include "pci.h"

uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outportl(PCI_CONFIG_ADDRESS, address);
    return inportl(PCI_CONFIG_DATA);
}

void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
    outportl(PCI_CONFIG_ADDRESS, address);
    outportl(PCI_CONFIG_DATA, value);
}

uint16_t pci_get_vendor(uint8_t bus, uint8_t slot) {
    return pci_read(bus, slot, 0, 0) & 0xFFFF;
}

uint16_t pci_get_device(uint8_t bus, uint8_t slot) {
    return (pci_read(bus, slot, 0, 0) >> 16) & 0xFFFF;
}

uint32_t pci_get_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_num) {
    return pci_read(bus, slot, func, 0x10 + 4 * bar_num);
}

void pci_check_all_buses(void) {
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint16_t vendor = pci_get_vendor(bus, slot);
            if (vendor != 0xFFFF) {
                uint16_t device = pci_get_device(bus, slot);
                log("[PCI] Found device %x:%x", 1, 0, vendor, device);
            }
        }
    }
}