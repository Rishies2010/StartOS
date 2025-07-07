#include "diskio.h"
#include "ata_pio_drv.h"
#include "../../libk/debug/log.h"

DSTATUS disk_status(BYTE pdrv)
{
    ata_drive_info_t* drive = ata_get_drive_info(pdrv);
    
    if (!drive || !drive->exists) {
        return STA_NODISK;
    }
    
    return 0;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) {
        return STA_NOINIT;
    }
    
    ata_status_t status = ata_init();
    if (status != ATA_OK) {
        log("Disk init failed", 1, 0);
        return STA_NOINIT;
    }
    
    ata_drive_info_t* drive = ata_get_drive_info(0);
    if (!drive || !drive->exists) {
        log("Drive 0 not found", 1, 0);
        return STA_NODISK;
    }
    
    log("Disk initialized", 4, 0);
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }
    
    if (!buff) {
        return RES_PARERR;
    }
    
    ata_drive_info_t* drive = ata_get_drive_info(0);
    if (!drive || !drive->exists) {
        return RES_NOTRDY;
    }
    
    ata_status_t status = ata_read_sectors(sector, count, buff);
    
    switch (status) {
        case ATA_OK:
            return RES_OK;
        case ATA_INVALID_PARAM:
            return RES_PARERR;
        case ATA_NO_DEVICE:
        case ATA_NOT_ATA:
            return RES_NOTRDY;
        case ATA_TIMEOUT:
        case ATA_ERROR:
        case ATA_DRIVE_FAULT:
        default:
            return RES_ERROR;
    }
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }
    
    if (!buff) {
        return RES_PARERR;
    }
    
    ata_drive_info_t* drive = ata_get_drive_info(0);
    if (!drive || !drive->exists) {
        return RES_NOTRDY;
    }
    
    ata_status_t status = ata_write_sectors_48(sector, count, buff);
    
    switch (status) {
        case ATA_OK:
            return RES_OK;
        case ATA_INVALID_PARAM:
            return RES_PARERR;
        case ATA_NO_DEVICE:
        case ATA_NOT_ATA:
            return RES_NOTRDY;
        case ATA_TIMEOUT:
        case ATA_ERROR:
        case ATA_DRIVE_FAULT:
        default:
            return RES_ERROR;
    }
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }
    
    ata_drive_info_t* drive = ata_get_drive_info(0);
    if (!drive || !drive->exists) {
        return RES_NOTRDY;
    }
    
    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;
            
        case GET_SECTOR_COUNT:
            if (buff) {
                *(LBA_t*)buff = (LBA_t)drive->sectors;
                return RES_OK;
            }
            return RES_PARERR;
            
        case GET_SECTOR_SIZE:
            if (buff) {
                *(WORD*)buff = 512;
                return RES_OK;
            }
            return RES_PARERR;
            
        case GET_BLOCK_SIZE:
            if (buff) {
                *(DWORD*)buff = 1;
                return RES_OK;
            }
            return RES_PARERR;
            
        default:
            return RES_PARERR;
    }
}