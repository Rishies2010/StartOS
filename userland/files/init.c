#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

int main(int argc, char **argv)
{
    for(;;)asm volatile("hlt");
    return 0;
}