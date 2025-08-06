#include "e1000.h"

static e1000_device dev;

static uint32_t e1000_read(uint32_t reg) {
    return *((volatile uint32_t *)(dev.mem_base + reg));
}

static void e1000_write(uint32_t reg, uint32_t value) {
    *((volatile uint32_t *)(dev.mem_base + reg)) = value;
}

static uint32_t e1000_eeprom_read(uint8_t addr) {
    uint32_t data = 0;
    uint32_t tmp = 0;
    
    if (dev.has_eeprom) {
        e1000_write(E1000_REG_EEPROM, 1 | (addr << 8));
        while (!((tmp = e1000_read(E1000_REG_EEPROM)) & (1 << 4)));
    } else {
        e1000_write(E1000_REG_EEPROM, 1 | (addr << 2));
        while (!((tmp = e1000_read(E1000_REG_EEPROM)) & (1 << 1)));
    }
    
    data = (tmp >> 16) & 0xFFFF;
    return data;
}

static uint8_t e1000_detect_eeprom(void) {
    e1000_write(E1000_REG_EEPROM, 1);
    for (int i = 0; i < 10000; i++) {
        uint32_t val = e1000_read(E1000_REG_EEPROM);
        if (val & (1 << 4)) {
            return 1;
        }
    }
    return 0;
}

static void e1000_reset(void) {
    uint32_t ctrl = e1000_read(E1000_REG_CTRL);
    e1000_write(E1000_REG_CTRL, ctrl | 0x04000000);
    while (e1000_read(E1000_REG_CTRL) & 0x04000000);
    for (int i = 0; i < 10000; i++);
}

static void e1000_read_mac(void) {
    if (dev.has_eeprom) {
        uint32_t temp = e1000_eeprom_read(0);
        dev.mac[0] = temp & 0xFF;
        dev.mac[1] = temp >> 8;
        temp = e1000_eeprom_read(1);
        dev.mac[2] = temp & 0xFF;
        dev.mac[3] = temp >> 8;
        temp = e1000_eeprom_read(2);
        dev.mac[4] = temp & 0xFF;
        dev.mac[5] = temp >> 8;
    } else {
        uint32_t mac_low = e1000_read(E1000_REG_RXADDR);
        uint32_t mac_high = e1000_read(E1000_REG_RXADDR2);
        dev.mac[0] = mac_low & 0xFF;
        dev.mac[1] = (mac_low >> 8) & 0xFF;
        dev.mac[2] = (mac_low >> 16) & 0xFF;
        dev.mac[3] = (mac_low >> 24) & 0xFF;
        dev.mac[4] = mac_high & 0xFF;
        dev.mac[5] = (mac_high >> 8) & 0xFF;
    }
    
    log("[E1000] MAC: %02x:%02x:%02x:%02x:%02x:%02x.", 1, 0,
        dev.mac[0], dev.mac[1], dev.mac[2], dev.mac[3], dev.mac[4], dev.mac[5]);
}

static void e1000_init_rx(void) {
    dev.rx_ring = (e1000_rx_desc *)kmalloc(NUM_RX_DESC * sizeof(e1000_rx_desc));
    dev.rx_buffers = (uint8_t *)kmalloc(NUM_RX_DESC * RX_BUFFER_SIZE);
    
    for (int i = 0; i < NUM_RX_DESC; i++) {
        dev.rx_ring[i].addr = (uint64_t)(dev.rx_buffers + i * RX_BUFFER_SIZE);
        dev.rx_ring[i].status = 0;
    }
    
    e1000_write(E1000_REG_RDBAL, (uint32_t)((uint64_t)dev.rx_ring & 0xFFFFFFFF));
    e1000_write(E1000_REG_RDBAH, (uint32_t)((uint64_t)dev.rx_ring >> 32));
    e1000_write(E1000_REG_RDLEN, NUM_RX_DESC * sizeof(e1000_rx_desc));
    e1000_write(E1000_REG_RDH, 0);
    e1000_write(E1000_REG_RDT, NUM_RX_DESC - 1);
    
    dev.rx_cur = 0;
    
    uint32_t rctl = E1000_RCTL_EN | E1000_RCTL_SBP | E1000_RCTL_UPE | 
                   E1000_RCTL_MPE | E1000_RCTL_LBM_NO | E1000_RCTL_RDMTS_HALF | 
                   E1000_RCTL_BAM | E1000_RCTL_SECRC | E1000_RCTL_LPE;
    
    e1000_write(E1000_REG_RCTL, rctl);
}

static void e1000_init_tx(void) {
    dev.tx_ring = (e1000_tx_desc *)kmalloc(NUM_TX_DESC * sizeof(e1000_tx_desc));
    dev.tx_buffers = (uint8_t *)kmalloc(NUM_TX_DESC * TX_BUFFER_SIZE);
    
    for (int i = 0; i < NUM_TX_DESC; i++) {
        dev.tx_ring[i].addr = (uint64_t)(dev.tx_buffers + i * TX_BUFFER_SIZE);
        dev.tx_ring[i].cmd = 0;
        dev.tx_ring[i].status = E1000_CMD_RS;
    }
    
    e1000_write(E1000_REG_TDBAL, (uint32_t)((uint64_t)dev.tx_ring & 0xFFFFFFFF));
    e1000_write(E1000_REG_TDBAH, (uint32_t)((uint64_t)dev.tx_ring >> 32));
    e1000_write(E1000_REG_TDLEN, NUM_TX_DESC * sizeof(e1000_tx_desc));
    e1000_write(E1000_REG_TDH, 0);
    e1000_write(E1000_REG_TDT, 0);
    
    dev.tx_cur = 0;
    
    uint32_t tctl = E1000_TCTL_EN | E1000_TCTL_PSP | 
                   (0x10 << E1000_TCTL_CT) | (0x40 << E1000_TCTL_COLD);
    
    e1000_write(E1000_REG_TCTL, tctl);
    e1000_write(E1000_REG_TIPG, 0x0060200A);
}

static void e1000_interrupt_handler() {
    e1000_handle_interrupt();
}

void e1000_init(void) {
    memset(&dev, 0, sizeof(e1000_device));
    
    for (int bus = 0; bus < 256; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            for (int func = 0; func < 8; func++) {
                uint16_t vendor = pci_get_vendor(bus, slot);
                if (vendor == 0xFFFF) continue;
                
                if (vendor == E1000_VENDOR_ID) {
                    uint16_t device = pci_get_device(bus, slot);
                    if (device == E1000_DEVICE_ID || device == E1000_DEVICE_ID_2 || device == E1000_DEVICE_ID_3) {
                        dev.bus = bus;
                        dev.slot = slot;
                        dev.func = func;
                        dev.device_id = device;
                        dev.bar0 = pci_get_bar(bus, slot, func, 0);
                        dev.mem_base = dev.bar0 & ~0xF;
                        
                        uint8_t irq = pci_read(bus, slot, func, 0x3C) & 0xFF;
                        dev.irq = irq;
                        
                        log("[E1000] Found device %04x at %02x:%02x.%x IRQ %d.", 1, 0, device, bus, slot, func, irq);
                        
                        pci_write(bus, slot, func, 0x04, 0x06);
                        
                        e1000_reset();
                        
                        dev.has_eeprom = e1000_detect_eeprom();
                        if (dev.has_eeprom) {
                            log("[E1000] EEPROM detected.", 1, 0);
                        } else {
                            log("[E1000] Using registers for MAC.", 1, 0);
                        }
                        
                        e1000_read_mac();
                        e1000_init_rx();
                        e1000_init_tx();
                        
                        e1000_write(E1000_REG_IMASK, 0x1F6DC);
                        e1000_write(E1000_REG_IMASK, 0xFF & ~4);
                        e1000_read(0xC0);
                        
                        register_interrupt_handler(32 + irq, e1000_interrupt_handler, "E1000 Network");
                        
                        log("[E1000] Initialized successfully.", 4, 0);
                        return;
                    }
                }
                
                if (func == 0) {
                    uint32_t header_type = pci_read(bus, slot, 0, 0x0C);
                    if (!(header_type & 0x800000)) break;
                }
            }
        }
    }
    log("[E1000] No compatible device found.", 3, 1);
}

void e1000_send_packet(void *data, uint16_t length) {
    if (length > TX_BUFFER_SIZE) {
        log("[E1000] Packet too large: %d bytes.", 3, 1, length);
        return;
    }
    
    dev.tx_ring[dev.tx_cur].length = length;
    dev.tx_ring[dev.tx_cur].status = 0;
    dev.tx_ring[dev.tx_cur].cmd = E1000_CMD_EOP | E1000_CMD_IFCS | E1000_CMD_RS;
    
    memcpy((void *)(dev.tx_buffers + dev.tx_cur * TX_BUFFER_SIZE), data, length);
    
    uint16_t old_cur = dev.tx_cur;
    dev.tx_cur = (dev.tx_cur + 1) % NUM_TX_DESC;
    e1000_write(E1000_REG_TDT, dev.tx_cur);
    
    while (!(dev.tx_ring[old_cur].status & 0xFF));
}

uint16_t e1000_receive_packet(void *buffer) {
    uint16_t idx = (e1000_read(E1000_REG_RDT) + 1) % NUM_RX_DESC;
    
    if (!(dev.rx_ring[idx].status & 0x1)) {
        return 0;
    }
    
    uint16_t length = dev.rx_ring[idx].length;
    if (length > RX_BUFFER_SIZE) {
        log("[E1000] Received oversized packet: %d bytes.", 3, 1, length);
        dev.rx_ring[idx].status = 0;
        e1000_write(E1000_REG_RDT, idx);
        return 0;
    }
    
    memcpy(buffer, (void *)(dev.rx_buffers + idx * RX_BUFFER_SIZE), length);
    dev.rx_ring[idx].status = 0;
    e1000_write(E1000_REG_RDT, idx);
    
    return length;
}

void e1000_get_mac_address(uint8_t *mac) {
    for (int i = 0; i < 6; i++) {
        mac[i] = dev.mac[i];
    }
}

uint32_t e1000_link_up(void) {
    uint32_t status = e1000_read(E1000_REG_STATUS);
    return (status & 2) ? 1 : 0;
}

void e1000_enable_interrupts(void) {
    e1000_write(E1000_REG_IMASK, 0x1F6DC);
}

void e1000_disable_interrupts(void) {
    e1000_write(E1000_REG_IMASK, 0);
}

uint32_t e1000_get_interrupt_status(void) {
    return e1000_read(0xC0);
}

void e1000_handle_interrupt(void) {
    uint32_t status = e1000_get_interrupt_status();
    
    if (status & 0x04) {
        log("[E1000] Link status changed.", 1, 0);
    }
    if (status & 0x10) {
        log("[E1000] Good threshold.", 1, 0);
    }
    if (status & 0x80) {
        log("[E1000] Receive packet.", 1, 0);
    }
    if (status & 0x01) {
        log("[E1000] Transmit packet.", 1, 0);
    }
}