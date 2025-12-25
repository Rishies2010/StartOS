#include "../userlib.h"

int main(void) {
    clear_screen();
    prints("Hello from userspace!\n");
    prints("Syscalls working!\n");
    
    printc('A');
    printc('\n');
    
    exit(0);
}