#ifndef ATA_PIO_DRV_H
#define ATA_PIO_DRV_H

#include "stdint.h"

// Return status codes
typedef enum {
    ATA_OK = 0,
    ATA_ERROR,
    ATA_TIMEOUT,
    ATA_NO_DEVICE,
    ATA_NOT_ATA,
    ATA_DRIVE_FAULT,
    ATA_UNSUPPORTED,
    ATA_INVALID_PARAM
} ata_status_t;

// Drive information structure
typedef struct {
    uint8_t exists;
    uint8_t drive_num;
    uint8_t lba28_supported;
    uint8_t lba48_supported;
    uint64_t sectors;
    char model[41];
    char serial[21];
} ata_drive_info_t;

// Function prototypes

/**
 * Initialize the ATA subsystem
 * @return ATA_OK on success, error code otherwise
 */
ata_status_t ata_init(void);

/**
 * Identify a specific drive
 * @param drive Drive number (0 = master, 1 = slave)
 * @param info Pointer to drive info structure to fill
 * @return ATA_OK on success, error code otherwise
 */
ata_status_t ata_identify_drive(uint8_t drive, ata_drive_info_t* info);

/**
 * Read sectors using 28-bit LBA
 * @param lba Starting LBA address (max 28-bit)
 * @param sector_count Number of sectors to read (1-256)
 * @param buffer Buffer to store read data (must be sector_count * 512 bytes)
 * @return ATA_OK on success, error code otherwise
 */
ata_status_t ata_read_sectors_28(uint32_t lba, uint8_t sector_count, uint8_t* buffer);

/**
 * Read sectors using 48-bit LBA
 * @param lba Starting LBA address (48-bit)
 * @param sector_count Number of sectors to read (1-65536)
 * @param buffer Buffer to store read data (must be sector_count * 512 bytes)
 * @return ATA_OK on success, error code otherwise
 */
ata_status_t ata_read_sectors_48(uint64_t lba, uint16_t sector_count, uint8_t* buffer);

/**
 * Write sectors using 48-bit LBA
 * @param lba Starting LBA address (48-bit)
 * @param sector_count Number of sectors to write (1-65536)
 * @param buffer Buffer containing data to write (must be sector_count * 512 bytes)
 * @return ATA_OK on success, error code otherwise
 */
ata_status_t ata_write_sectors_48(uint64_t lba, uint16_t sector_count, const uint8_t* buffer);

/**
 * Generic read function that automatically chooses LBA28/48
 * @param lba Starting LBA address
 * @param sector_count Number of sectors to read
 * @param buffer Buffer to store read data
 * @return ATA_OK on success, error code otherwise
 */
ata_status_t ata_read_sectors(uint64_t lba, uint16_t sector_count, uint8_t* buffer);

/**
 * Get drive information
 * @param drive Drive number (0 = master, 1 = slave)
 * @return Pointer to drive info structure, NULL if invalid drive
 */
ata_drive_info_t* ata_get_drive_info(uint8_t drive);

#endif