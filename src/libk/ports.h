#ifndef PORTS_H
#define PORTS_H
#include <stdint.h>

unsigned char inportb(unsigned short port);
void outportb(unsigned short port, unsigned char data);
void outportw(uint16_t port, uint16_t val);
uint16_t inportw(uint16_t port);
void outportl(uint16_t port, uint32_t val);
uint32_t inportl(uint16_t port);

#endif