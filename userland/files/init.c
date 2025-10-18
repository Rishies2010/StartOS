#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "../kernel_api.h"
static kernel_api_t* g_api = NULL;

int main(int argc, char **argv, kernel_api_t* api)
{
    g_api = api;
    if(exec("shell", 0, NULL) != SFS_OK)
        log("No shell program found.", 0, 0);
    return 0;
}