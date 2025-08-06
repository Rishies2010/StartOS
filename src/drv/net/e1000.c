#include "e1000.h"

static e1000_device dev;

static uint32_t e1000_read(uint32_t reg) {
    return *((volatile uint32_t *)(dev.mem_base + reg));
}

static void e1000_write(uint32_t reg, uint32_t value) {
    *((volatile uint32_t *)(dev.mem_base + reg)) = value;
}

static void e1000_reset() {
    e1000_write(E1000_REG_CTRL, e1000_read(E1000_REG_CTRL) | 0x04000000);
    while (e1000_read(E1000_REG_CTRL) & 0x04000000);
}

static void e1000_set_mac() {
    uint32_t mac_low = e1000_read(0x5400);
    uint32_t mac_high = e1000_read(0x5404);
    log("[E1000] MAC: %x:%x:%x:%x:%x:%x", 1, 0, 
        mac_high >> 8, mac_high & 0xFF, mac_low >> 24, (mac_low >> 16) & 0xFF, 
        (mac_low >> 8) & 0xFF, mac_low & 0xFF);
}

static void e1000_init_rx() {
    dev.rx_ring = (e1000_rx_desc *)kmalloc(NUM_RX_DESC * sizeof(e1000_rx_desc));
    dev.rx_buffers = (uint8_t *)kmalloc(NUM_RX_DESC * RX_BUFFER_SIZE);
    
    for (int i = 0; i < NUM_RX_DESC; i++) {
        dev.rx_ring[i].addr = (uint64_t)(dev.rx_buffers + i * RX_BUFFER_SIZE);
        dev.rx_ring[i].status = 0;
    }
    
    e1000_write(E1000_REG_RDBAL, (uint32_t)dev.rx_ring);
    e1000_write(E1000_REG_RDBAH, 0);
    e1000_write(E1000_REG_RDLEN, NUM_RX_DESC * sizeof(e1000_rx_desc));
    e1000_write(E1000_REG_RDH, 0);
    e1000_write(E1000_REG_RDT, NUM_RX_DESC - 1);
    
    uint32_t rctl = e1000_read(E1000_REG_RCTL);
    rctl |= E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC;
    e1000_write(E1000_REG_RCTL, rctl);
}

static void e1000_init_tx() {
    dev.tx_ring = (e1000_tx_desc *)kmalloc(NUM_TX_DESC * sizeof(e1000_tx_desc));
    dev.tx_buffers = (uint8_t *)kmalloc(NUM_TX_DESC * TX_BUFFER_SIZE);
    
    for (int i = 0; i < NUM_TX_DESC; i++) {
        dev.tx_ring[i].addr = (uint64_t)(dev.tx_buffers + i * TX_BUFFER_SIZE);
        dev.tx_ring[i].cmd = 0;
        dev.tx_ring[i].status = 0;
    }
    
    e1000_write(E1000_REG_TDBAL, (uint32_t)dev.tx_ring);
    e1000_write(E1000_REG_TDBAH, 0);
    e1000_write(E1000_REG_TDLEN, NUM_TX_DESC * sizeof(e1000_tx_desc));
    e1000_write(E1000_REG_TDH, 0);
    e1000_write(E1000_REG_TDT, 0);
    
    uint32_t tctl = e1000_read(E1000_REG_TCTL);
    tctl |= E1000_TCTL_EN | E1000_TCTL_PSP;
    tctl |= (0x10 << E1000_TCTL_CT) | (0x40 << E1000_TCTL_COLD);
    e1000_write(E1000_REG_TCTL, tctl);
    e1000_write(E1000_REG_TIPG, 0x0060200A);
}

void e1000_init() {
    
    for (uint8_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            uint16_t vendor = pci_get_vendor(bus, slot);
            if (vendor == E1000_VENDOR_ID) {
                uint16_t device = pci_get_device(bus, slot);
                if (device == E1000_DEVICE_ID) {
                    dev.bus = bus;
                    dev.slot = slot;
                    dev.func = 0;
                    dev.bar0 = pci_get_bar(bus, slot, 0, 0);
                    dev.mem_base = dev.bar0 & ~0xF;
                    log("[E1000] Found at %x:%x", 1, 0, bus, slot);
                    
                    e1000_reset();
                    e1000_set_mac();
                    e1000_init_rx();
                    e1000_init_tx();
                    
                    log("[E1000] Initialized", 4, 0);
                    return;
                }
            }
        }
    }
    log("[E1000] Not found", 3, 1);
}

void e1000_send_packet(void *data, uint16_t length) {
    uint16_t idx = dev.tx_cur;
    memcpy((void *)dev.tx_ring[idx].addr, data, length);
    dev.tx_ring[idx].length = length;
    dev.tx_ring[idx].cmd = 0x9;
    dev.tx_ring[idx].status = 0;
    
    dev.tx_cur = (dev.tx_cur + 1) % NUM_TX_DESC;
    e1000_write(E1000_REG_TDT, dev.tx_cur);
}

uint16_t e1000_receive_packet(void *buffer) {
    uint16_t idx = (e1000_read(E1000_REG_RDT) + 1) % NUM_RX_DESC;
    if (!(dev.rx_ring[idx].status & 0x1)) {
        return 0;
    }
    
    uint16_t length = dev.rx_ring[idx].length;
    memcpy(buffer, (void *)dev.rx_ring[idx].addr, length);
    dev.rx_ring[idx].status = 0;
    e1000_write(E1000_REG_RDT, idx);
    return length;
}