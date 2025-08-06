#include "tcp.h"

static uint16_t tcp_checksum(void *data, uint16_t length) {
    uint32_t sum = 0;
    uint16_t *ptr = data;
    
    for (; length > 1; length -= 2) {
        sum += *ptr++;
    }
    
    if (length) {
        sum += *(uint8_t *)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

void tcp_send(uint16_t src_port, uint16_t dst_port, uint32_t dst_ip, void *data, uint16_t length) {
    uint8_t packet[1500];
    tcp_header_t *tcp = (tcp_header_t *)packet;
    
    tcp->src_port = src_port;
    tcp->dst_port = dst_port;
    tcp->seq_num = 0;
    tcp->ack_num = 0;
    tcp->data_offset = 5 << 4;
    tcp->flags = TCP_ACK;
    tcp->window = 0xFFFF;
    tcp->urgent_ptr = 0;
    
    memcpy(packet + sizeof(tcp_header_t), data, length);
    tcp->checksum = tcp_checksum(packet, sizeof(tcp_header_t) + length);
    
    e1000_send_packet(packet, sizeof(tcp_header_t) + length);
}

void tcp_handle_packet(void *packet, uint16_t length) {
    tcp_header_t *tcp = packet;
    uint8_t *data = packet + sizeof(tcp_header_t);
    uint16_t data_len = length - sizeof(tcp_header_t);
    
    if (tcp_checksum(packet, length) != 0) {
        log("[TCP] Bad checksum", 3, 0);
        return;
    }
    
    if (tcp->flags & TCP_SYN) {
        log("[TCP] SYN received", 1, 0);
    } else if (tcp->flags & TCP_FIN) {
        log("[TCP] FIN received", 1, 0);
    } else if (tcp->flags & TCP_ACK) {
        log("[TCP] ACK received", 1, 0);
    }
}