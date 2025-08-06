#ifndef E1000_H
#define E1000_H

#include <stdint.h>
#include "pci.h"
#include "../../libk/core/mem.h"
#include "../../libk/debug/log.h"
#include "../../libk/string.h"

#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID 0x100E

#define E1000_REG_CTRL     0x0000
#define E1000_REG_STATUS   0x0008
#define E1000_REG_EERD     0x0014
#define E1000_REG_CTRL_EXT 0x0018
#define E1000_REG_IMASK    0x00D0
#define E1000_REG_RCTL     0x0100
#define E1000_REG_TCTL     0x0400
#define E1000_REG_TIPG     0x0410
#define E1000_REG_RDBAL    0x2800
#define E1000_REG_RDBAH    0x2804
#define E1000_REG_RDLEN    0x2808
#define E1000_REG_RDH      0x2810
#define E1000_REG_RDT      0x2818
#define E1000_REG_TDBAL    0x3800
#define E1000_REG_TDBAH    0x3804
#define E1000_REG_TDLEN    0x3808
#define E1000_REG_TDH      0x3810
#define E1000_REG_TDT      0x3818

#define E1000_RCTL_EN      (1 << 1)
#define E1000_RCTL_SBP     (1 << 2)
#define E1000_RCTL_UPE     (1 << 3)
#define E1000_RCTL_MPE     (1 << 4)
#define E1000_RCTL_LPE     (1 << 5)
#define E1000_RCTL_LBM     (3 << 6)
#define E1000_RCTL_RDMTS   (3 << 8)
#define E1000_RCTL_MO      (3 << 12)
#define E1000_RCTL_BAM     (1 << 15)
#define E1000_RCTL_BSIZE   (3 << 16)
#define E1000_RCTL_SECRC   (1 << 26)

#define E1000_TCTL_EN      (1 << 1)
#define E1000_TCTL_PSP     (1 << 3)
#define E1000_TCTL_CT      (0xFF << 4)
#define E1000_TCTL_COLD    (0x3FF << 12)
#define E1000_TCTL_SWXOFF  (1 << 22)
#define E1000_TCTL_RTLC    (1 << 24)

#define NUM_RX_DESC 32
#define NUM_TX_DESC 8
#define RX_BUFFER_SIZE 2048
#define TX_BUFFER_SIZE 2048

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} e1000_rx_desc;

typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} e1000_tx_desc;

typedef struct {
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t mem_base;
    e1000_rx_desc *rx_ring;
    e1000_tx_desc *tx_ring;
    uint8_t *rx_buffers;
    uint8_t *tx_buffers;
    uint16_t rx_cur;
    uint16_t tx_cur;
} e1000_device;

void e1000_init();
void e1000_send_packet(void *data, uint16_t length);
uint16_t e1000_receive_packet(void *buffer);

#endif