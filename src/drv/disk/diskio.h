#ifndef DISKIO_H
#define DISKIO_H

#include <stdint.h>
#include "ff.h"

// Disk status definitions (FatFs standard)
#define STA_NOINIT      0x01    // Drive not initialized
#define STA_NODISK      0x02    // No medium in the drive
#define STA_PROTECT     0x04    // Write protected

// Disk operation result codes (FatFs standard)
typedef enum {
    RES_OK = 0,     // Successful
    RES_ERROR,      // R/W Error
    RES_WRPRT,      // Write Protected
    RES_NOTRDY,     // Not Ready
    RES_PARERR      // Invalid Parameter
} DRESULT;

// Disk status type
typedef BYTE DSTATUS;

// Control commands for disk_ioctl()
#define CTRL_SYNC           0   // Complete pending write process
#define GET_SECTOR_COUNT    1   // Get media size (for only f_mkfs())
#define GET_SECTOR_SIZE     2   // Get sector size (for multiple sector size)
#define GET_BLOCK_SIZE      3   // Get erase block size (for only f_mkfs())

// Function prototypes
DSTATUS disk_status(BYTE pdrv);
DSTATUS disk_initialize(BYTE pdrv);
DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);

#endif // DISKIO_H