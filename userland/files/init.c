#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../kernel_api.h"

int main(kernel_api_t* api){
    log("Init completed. %i", 4, 0, strlen("Hello!"));
    return 0;
}