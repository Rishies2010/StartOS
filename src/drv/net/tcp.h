#ifndef TCP_H
#define TCP_H

#include <stdint.h>
#include "e1000.h"

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10
#define TCP_URG 0x20

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent_ptr;
} tcp_header_t;

void tcp_send(uint16_t src_port, uint16_t dst_port, uint32_t dst_ip, void *data, uint16_t length);
void tcp_handle_packet(void *packet, uint16_t length);

#endif