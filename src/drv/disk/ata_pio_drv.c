#include "ata_pio_drv.h"
#include "../../libk/ports.h"
#include "../vga.h"
#include "../../libk/string.h"
#include "../../libk/debug/log.h"
#include <stddef.h>

#define ATA_PRIMARY_DATA         0x1F0
#define ATA_PRIMARY_ERR          0x1F1
#define ATA_PRIMARY_SECCOUNT     0x1F2
#define ATA_PRIMARY_LBA_LO       0x1F3
#define ATA_PRIMARY_LBA_MID      0x1F4
#define ATA_PRIMARY_LBA_HI       0x1F5
#define ATA_PRIMARY_DRIVE_HEAD   0x1F6
#define ATA_PRIMARY_COMM_REGSTAT 0x1F7
#define ATA_PRIMARY_ALTSTAT_DCR  0x3F6

// Status register bits
#define STAT_ERR  (1 << 0) // Error occurred
#define STAT_IDX  (1 << 1) // Index (always 0)
#define STAT_CORR (1 << 2) // Corrected data (always 0)
#define STAT_DRQ  (1 << 3) // Data request ready
#define STAT_SRV  (1 << 4) // Overlapped mode service request
#define STAT_DF   (1 << 5) // Drive fault error
#define STAT_RDY  (1 << 6) // Drive ready
#define STAT_BSY  (1 << 7) // Drive busy

// Error register bits
#define ERR_AMNF  (1 << 0) // Address mark not found
#define ERR_TK0NF (1 << 1) // Track 0 not found
#define ERR_ABRT  (1 << 2) // Command aborted
#define ERR_MCR   (1 << 3) // Media change request
#define ERR_IDNF  (1 << 4) // ID not found
#define ERR_MC    (1 << 5) // Media changed
#define ERR_UNC   (1 << 6) // Uncorrectable data error
#define ERR_BBK   (1 << 7) // Bad block detected

// Commands
#define CMD_READ_SECTORS     0x20
#define CMD_READ_SECTORS_EXT 0x24
#define CMD_WRITE_SECTORS    0x30
#define CMD_WRITE_SECTORS_EXT 0x34
#define CMD_CACHE_FLUSH      0xE7
#define CMD_CACHE_FLUSH_EXT  0xEA
#define CMD_IDENTIFY         0xEC

// Drive selection
#define DRIVE_MASTER 0xA0
#define DRIVE_SLAVE  0xB0
#define DRIVE_LBA    0xE0  // LBA mode master
#define DRIVE_LBA48  0x40  // LBA48 mode

// Timeouts (in polling loops)
#define ATA_TIMEOUT 1000000
#define ATA_IDENTIFY_TIMEOUT 5000000

// Global drive info
static ata_drive_info_t primary_master = {0};
static ata_drive_info_t primary_slave = {0};

/**
 * Wait for drive to be ready (BSY clear, RDY set)
 */
static ata_status_t ata_wait_ready(uint32_t timeout) {
    uint32_t loops = 0;
    uint8_t status;
    
    while (loops < timeout) {
        status = inportb(ATA_PRIMARY_COMM_REGSTAT);
        
        if (!(status & STAT_BSY) && (status & STAT_RDY)) {
            return ATA_OK;
        }
        
        if (status & STAT_ERR) {
            return ATA_ERROR;
        }
        
        if (status & STAT_DF) {
            return ATA_DRIVE_FAULT;
        }
        
        loops++;
    }
    
    return ATA_TIMEOUT;
}

/**
 * Wait for data request ready
 */
static ata_status_t ata_wait_drq(uint32_t timeout) {
    uint32_t loops = 0;
    uint8_t status;
    
    while (loops < timeout) {
        status = inportb(ATA_PRIMARY_COMM_REGSTAT);
        
        if (status & STAT_DRQ) {
            return ATA_OK;
        }
        
        if (status & STAT_ERR) {
            return ATA_ERROR;
        }
        
        if (status & STAT_DF) {
            return ATA_DRIVE_FAULT;
        }
        
        loops++;
    }
    
    return ATA_TIMEOUT;
}

/**
 * Software reset of ATA controller
 */
static void ata_soft_reset(void) {
    outportb(ATA_PRIMARY_ALTSTAT_DCR, 0x04); // Set SRST bit
    
    // Wait at least 5μs (we'll wait much longer)
    for (volatile int i = 0; i < 1000; i++);
    
    outportb(ATA_PRIMARY_ALTSTAT_DCR, 0x00); // Clear SRST bit
    
    // Wait for drives to be ready
    ata_wait_ready(ATA_TIMEOUT);
}

/**
 * Get detailed error information
 */
static const char* ata_get_error_string(uint8_t error) {
    if (error & ERR_BBK)   return "Bad block detected";
    if (error & ERR_UNC)   return "Uncorrectable data error";
    if (error & ERR_MC)    return "Media changed";
    if (error & ERR_IDNF)  return "ID not found";
    if (error & ERR_MCR)   return "Media change request";
    if (error & ERR_ABRT)  return "Command aborted";
    if (error & ERR_TK0NF) return "Track 0 not found";
    if (error & ERR_AMNF)  return "Address mark not found";
    return "Unknown error";
}

/**
 * IDENTIFY drives yeh
 */
ata_status_t ata_identify_drive(uint8_t drive, ata_drive_info_t* info) {
    if (!info) return ATA_INVALID_PARAM;
    
    log("ATA: Identifying drive...", 1, 0);
    
    // Select drive
    uint8_t drive_select = (drive == 0) ? DRIVE_MASTER : DRIVE_SLAVE;
    outportb(ATA_PRIMARY_DRIVE_HEAD, drive_select);
    
    // Small delay after drive selection
    for (volatile int i = 0; i < 100; i++);
    
    // Clear registers
    outportb(ATA_PRIMARY_SECCOUNT, 0);
    outportb(ATA_PRIMARY_LBA_LO, 0);
    outportb(ATA_PRIMARY_LBA_MID, 0);
    outportb(ATA_PRIMARY_LBA_HI, 0);
    
    // Send IDENTIFY command
    outportb(ATA_PRIMARY_COMM_REGSTAT, CMD_IDENTIFY);
    
    // Read status immediately
    uint8_t status = inportb(ATA_PRIMARY_COMM_REGSTAT);
    if (status == 0) {
        log("ATA: Drive does not exist", 2, 0);
        return ATA_NO_DEVICE;
    }
    
    // Wait for BSY to clear
    ata_status_t result = ata_wait_ready(ATA_IDENTIFY_TIMEOUT);
    if (result != ATA_OK) {
        log("ATA: Timeout waiting for drive ready", 3, 0);
        return result;
    }
    
    // Check if this is an ATAPI device
    uint8_t mid = inportb(ATA_PRIMARY_LBA_MID);
    uint8_t hi = inportb(ATA_PRIMARY_LBA_HI);
    if (mid != 0 || hi != 0) {
        log("ATA: Device is not ATA (possibly ATAPI)", 2, 0);
        return ATA_NOT_ATA;
    }
    
    // Wait for DRQ or ERR
    result = ata_wait_drq(ATA_TIMEOUT);
    if (result != ATA_OK) {
        uint8_t error = inportb(ATA_PRIMARY_ERR);
        log("ATA: Error during identify: %s", 3, 0), ata_get_error_string(error);
        return result;
    }
    
    // Read IDENTIFY data
    uint16_t identify_data[256];
    inportsw(ATA_PRIMARY_DATA, (uint8_t*)identify_data, 256);
    
    // Parse important information
    info->exists = 1;
    info->drive_num = drive;
    
    // LBA28/48 support
    info->lba28_supported = (identify_data[49] & (1 << 9)) != 0;
    info->lba48_supported = (identify_data[83] & (1 << 10)) != 0;
    
    // Sector counts
    if (info->lba48_supported) {
        info->sectors = ((uint64_t)identify_data[103] << 48) |
                       ((uint64_t)identify_data[102] << 32) |
                       ((uint64_t)identify_data[101] << 16) |
                       ((uint64_t)identify_data[100]);
    } else {
        info->sectors = ((uint32_t)identify_data[61] << 16) | identify_data[60];
    }
    
    // Model string (words 27-46, 40 bytes)
    for (int i = 0; i < 40; i += 2) {
        info->model[i] = identify_data[27 + i/2] >> 8;
        info->model[i+1] = identify_data[27 + i/2] & 0xFF;
    }
    info->model[40] = '\0';
    
    // Serial number (words 10-19, 20 bytes)
    for (int i = 0; i < 20; i += 2) {
        info->serial[i] = identify_data[10 + i/2] >> 8;
        info->serial[i+1] = identify_data[10 + i/2] & 0xFF;
    }
    info->serial[20] = '\0';
    
    log("ATA: Drive identified successfully", 1, 0);
    log("ATA: Model: %s", 1, 0, info->model);
    if(strstr(info->model, "QEMU") != NULL)
        log("StartOS running in a VM (QEMU)", 4, 0);
    if(strstr(info->model, "VBOX") != NULL)
        log("StartOS running in a VM (VirtualBox)", 4, 0);
    return ATA_OK;
}

/**
 * Initialize ATA subsystem
 */
ata_status_t ata_init(void) {
    log("ATA: Initializing ATA subsystem", 1, 0);
    ata_soft_reset();
    ata_identify_drive(0, &primary_master);
    ata_identify_drive(1, &primary_slave);
    
    if (!primary_master.exists && !primary_slave.exists) {
        log("ATA: No drives found", 2, 0);
        return ATA_NO_DEVICE;
    }
    log("ATA: Initialization complete", 1, 0);
    return ATA_OK;
}

/**
 * cool 28-bit LBA read
 */
ata_status_t ata_read_sectors_28(uint32_t lba, uint8_t sector_count, uint8_t* buffer) {
    if (!buffer || sector_count == 0) return ATA_INVALID_PARAM;
    if (!primary_master.exists) return ATA_NO_DEVICE;
    if (!primary_master.lba28_supported) return ATA_UNSUPPORTED;
    if (lba > 0x0FFFFFFF) return ATA_INVALID_PARAM; // 28-bit limit
    
    // Wait for drive ready
    ata_status_t result = ata_wait_ready(ATA_TIMEOUT);
    if (result != ATA_OK) return result;
    
    // Set up registers
    outportb(ATA_PRIMARY_DRIVE_HEAD, DRIVE_LBA | ((lba >> 24) & 0x0F));
    outportb(ATA_PRIMARY_ERR, 0x00);
    outportb(ATA_PRIMARY_SECCOUNT, sector_count);
    outportb(ATA_PRIMARY_LBA_LO, lba & 0xFF);
    outportb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outportb(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);
    
    // Send read command
    outportb(ATA_PRIMARY_COMM_REGSTAT, CMD_READ_SECTORS);
    
    // Read sectors
    for (uint8_t i = 0; i < sector_count; i++) {
        result = ata_wait_drq(ATA_TIMEOUT);
        if (result != ATA_OK) {
            uint8_t error = inportb(ATA_PRIMARY_ERR);
            log("ATA: Read error: %s", 3, 0, ata_get_error_string(error));
            return result;
        }
        
        inportsw(ATA_PRIMARY_DATA, buffer, 256);
        buffer += 512; // 512 bytes per sector
    }
    
    return ATA_OK;
}

/**
 * cool 48-bit LBA read
 */
ata_status_t ata_read_sectors_48(uint64_t lba, uint16_t sector_count, uint8_t* buffer) {
    if (!buffer || sector_count == 0) return ATA_INVALID_PARAM;
    if (!primary_master.exists) return ATA_NO_DEVICE;
    if (!primary_master.lba48_supported) return ATA_UNSUPPORTED;
    
    // Wait for drive ready
    ata_status_t result = ata_wait_ready(ATA_TIMEOUT);
    if (result != ATA_OK) return result;
    
    // Set up registers (48-bit LBA requires writing twice)
    outportb(ATA_PRIMARY_DRIVE_HEAD, DRIVE_LBA48);
    
    // High bytes first
    outportb(ATA_PRIMARY_SECCOUNT, (sector_count >> 8) & 0xFF);
    outportb(ATA_PRIMARY_LBA_LO, (lba >> 24) & 0xFF);
    outportb(ATA_PRIMARY_LBA_MID, (lba >> 32) & 0xFF);
    outportb(ATA_PRIMARY_LBA_HI, (lba >> 40) & 0xFF);
    
    // Low bytes second
    outportb(ATA_PRIMARY_SECCOUNT, sector_count & 0xFF);
    outportb(ATA_PRIMARY_LBA_LO, lba & 0xFF);
    outportb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outportb(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);
    
    // Send read command
    outportb(ATA_PRIMARY_COMM_REGSTAT, CMD_READ_SECTORS_EXT);
    
    // Read sectors
    for (uint16_t i = 0; i < sector_count; i++) {
        result = ata_wait_drq(ATA_TIMEOUT);
        if (result != ATA_OK) {
            uint8_t error = inportb(ATA_PRIMARY_ERR);
            log("ATA: Read error: %s", 3, 0, ata_get_error_string(error));
            return result;
        }
        
        inportsw(ATA_PRIMARY_DATA, buffer, 256);
        buffer += 512;
    }
    
    return ATA_OK;
}

/**
 * Enhanced 48-bit LBA write
 */
ata_status_t ata_write_sectors_48(uint64_t lba, uint16_t sector_count, const uint8_t* buffer) {
    if (!buffer || sector_count == 0) return ATA_INVALID_PARAM;
    if (!primary_master.exists) return ATA_NO_DEVICE;
    if (!primary_master.lba48_supported) return ATA_UNSUPPORTED;
    
    // Wait for drive ready
    ata_status_t result = ata_wait_ready(ATA_TIMEOUT);
    if (result != ATA_OK) return result;
    
    // Set up registers
    outportb(ATA_PRIMARY_DRIVE_HEAD, DRIVE_LBA48);
    
    // High bytes first
    outportb(ATA_PRIMARY_SECCOUNT, (sector_count >> 8) & 0xFF);
    outportb(ATA_PRIMARY_LBA_LO, (lba >> 24) & 0xFF);
    outportb(ATA_PRIMARY_LBA_MID, (lba >> 32) & 0xFF);
    outportb(ATA_PRIMARY_LBA_HI, (lba >> 40) & 0xFF);
    
    // Low bytes second
    outportb(ATA_PRIMARY_SECCOUNT, sector_count & 0xFF);
    outportb(ATA_PRIMARY_LBA_LO, lba & 0xFF);
    outportb(ATA_PRIMARY_LBA_MID, (lba >> 8) & 0xFF);
    outportb(ATA_PRIMARY_LBA_HI, (lba >> 16) & 0xFF);
    
    // Send write command
    outportb(ATA_PRIMARY_COMM_REGSTAT, CMD_WRITE_SECTORS_EXT);
    
    // Write sectors
    for (uint16_t i = 0; i < sector_count; i++) {
        result = ata_wait_drq(ATA_TIMEOUT);
        if (result != ATA_OK) {
            uint8_t error = inportb(ATA_PRIMARY_ERR);
            log("ATA: Write error: %s", 3, 0, ata_get_error_string(error));
            return result;
        }
        
        outportsw(ATA_PRIMARY_DATA, (uint8_t*)buffer, 256);
        buffer += 512;
    }
    
    // Flush cache
    outportb(ATA_PRIMARY_COMM_REGSTAT, primary_master.lba48_supported ? 
             CMD_CACHE_FLUSH_EXT : CMD_CACHE_FLUSH);
    
    // Wait for flush complete
    result = ata_wait_ready(ATA_TIMEOUT);
    if (result != ATA_OK) {
        log("ATA: Cache flush failed", 3, 0);
        return result;
    }
    
    return ATA_OK;
}

/**
 * Get drive information
 */
ata_drive_info_t* ata_get_drive_info(uint8_t drive) {
    if (drive == 0) return &primary_master;
    if (drive == 1) return &primary_slave;
    return NULL;
}

/**
 * Generic read function that automatically chooses LBA28/48
 */
ata_status_t ata_read_sectors(uint64_t lba, uint16_t sector_count, uint8_t* buffer) {
    if (!primary_master.exists) return ATA_NO_DEVICE;
    
    // Choose appropriate LBA mode
    if (lba <= 0x0FFFFFFF && sector_count <= 256 && primary_master.lba28_supported) {
        return ata_read_sectors_28((uint32_t)lba, (uint8_t)sector_count, buffer);
    } else if (primary_master.lba48_supported) {
        return ata_read_sectors_48(lba, sector_count, buffer);
    } else {
        return ATA_UNSUPPORTED;
    }
}