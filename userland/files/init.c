#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "../kernel_api.h"
static kernel_api_t* g_api = NULL;

int main(kernel_api_t* api)
{
    g_api = api;
    exec("shell");
    for(;;)
        ;
    return 0;
}