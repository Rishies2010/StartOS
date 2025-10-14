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
    if(exec("shell") != SFS_OK)
        log("No shell program found.", 0, 0);
    return 0;
}