#include "pic.h"
#include "../libk/ports.h"
#include "../libk/debug/log.h"

#define MASTER_COMMAND 0x20
#define MASTER_DATA 0x21
#define SLAVE_COMMAND 0xA0
#define SLAVE_DATA 0xA1

#define ICW1_ICW4 0x01       /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02     /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04  /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08      /* Level triggered (edge) mode */
#define ICW1_INIT 0x10       /* Initialization - required! */

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

unsigned int io_wait()
{
    int j = 0;
    for(unsigned int i = 0; i < 0xFFFFFFFF; i++)
    {
        j = i;
    }
    return j;
}

void remap_pic(void)
{
    outportb(MASTER_COMMAND, ICW1_INIT | ICW1_ICW4); // Start initialization sequence
    outportb(SLAVE_COMMAND, ICW1_INIT | ICW1_ICW4);  // Start initialization sequence
    outportb(MASTER_DATA, 0x20);                     // Remap to 0x20
    outportb(SLAVE_DATA, 0x28);                      // Remap slave to 0x28
    outportb(MASTER_DATA, 4);                        // Tell Master about slave @ IRQ 2
    outportb(SLAVE_DATA, 2);                         // Tell slave PIC its cascade ID
    outportb(MASTER_DATA, ICW4_8086);
    outportb(SLAVE_DATA, ICW4_8086);
    outportb(MASTER_DATA, 0x0);
    outportb(SLAVE_DATA, 0x0);
    log("PIC Remapped successfully.", 4, 0);
}

void send_eoi(unsigned char irq) {
    if (irq >= 8) outportb(SLAVE_COMMAND, 0x20);
    outportb(MASTER_COMMAND, 0x20);
}