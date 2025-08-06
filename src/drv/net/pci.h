#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include "../../libk/ports.h"
#include "../../libk/debug/log.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

uint32_t pci_read(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_write(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);
uint16_t pci_get_vendor(uint8_t bus, uint8_t slot);
uint16_t pci_get_device(uint8_t bus, uint8_t slot);
void pci_check_all_buses(void);
uint32_t pci_get_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_num);

#endif