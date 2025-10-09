#include "sfs.h"
#include "../disk/ata.h"
#include "../../libk/core/mem.h"
#include "../../libk/string.h"
#include "../../libk/debug/log.h"

static sfs_superblock_t superblock;
static sfs_file_entry_t file_table[SFS_MAX_FILES];
static uint8_t initialized = 0;

static uint32_t block_to_lba(uint32_t block)
{

    return SFS_DATA_START_LBA + (block * 8);
}

static sfs_error_t write_superblock(void)
{
    if (ata_write_sectors(superblock.drive_number, SFS_SUPERBLOCK_LBA, 1,
                          &superblock) != ATA_SUCCESS)
    {
        return SFS_ERR_WRITE_FAILED;
    }
    return SFS_OK;
}

static sfs_error_t write_file_table(void)
{
    if (ata_write_sectors(superblock.drive_number, SFS_FILETABLE_LBA,
                          SFS_FILETABLE_SIZE, file_table) != ATA_SUCCESS)
    {
        return SFS_ERR_WRITE_FAILED;
    }
    return SFS_OK;
}

sfs_error_t sfs_format(uint8_t drive)
{
    if (drive >= 4)
    {
        return SFS_ERR_INVALID_DRIVE;
    }

    if (ata_drive_exists(drive) != ATA_SUCCESS)
    {
        return SFS_ERR_INVALID_DRIVE;
    }

    log("StartFS: Formatting drive %d...", 1, 0, drive);

    memset(&superblock, 0, sizeof(sfs_superblock_t));
    superblock.magic = SFS_MAGIC;
    superblock.drive_number = drive;

    uint32_t total_sectors = 1048576;
    uint32_t data_sectors = total_sectors - SFS_DATA_START_LBA;
    superblock.total_blocks = data_sectors / 8;
    superblock.free_blocks = superblock.total_blocks;
    superblock.file_count = 0;

    memset(file_table, 0, sizeof(file_table));

    if (write_superblock() != SFS_OK)
    {
        log("StartFS: Failed to write superblock", 3, 1);
        return SFS_ERR_WRITE_FAILED;
    }

    if (write_file_table() != SFS_OK)
    {
        log("StartFS: Failed to write file table", 3, 1);
        return SFS_ERR_WRITE_FAILED;
    }

    log("StartFS: Format complete! %d blocks available", 4, 0, superblock.total_blocks);
    return SFS_OK;
}

sfs_error_t sfs_init(uint8_t drive)
{
    if (drive >= 4)
    {
        return SFS_ERR_INVALID_DRIVE;
    }

    if (ata_drive_exists(drive) != ATA_SUCCESS)
    {
        return SFS_ERR_INVALID_DRIVE;
    }

    if (ata_read_sectors(drive, SFS_SUPERBLOCK_LBA, 1, &superblock) != ATA_SUCCESS)
    {
        log("StartFS: Failed to read superblock", 3, 1);
        return SFS_ERR_READ_FAILED;
    }

    if (superblock.magic != SFS_MAGIC)
    {
        log("StartFS: Invalid magic number (expected 0x%x, got 0x%x)", 3, 1,
            SFS_MAGIC, superblock.magic);
        return SFS_ERR_NOT_SFS;
    }

    if (ata_read_sectors(drive, SFS_FILETABLE_LBA, SFS_FILETABLE_SIZE,
                         file_table) != ATA_SUCCESS)
    {
        log("StartFS: Failed to read file table", 3, 1);
        return SFS_ERR_READ_FAILED;
    }

    initialized = 1;

    log("StartFS: Initialized successfully", 4, 0);
    log("  Total blocks: %d (%d KB)", 1, 0,
        superblock.total_blocks, (superblock.total_blocks * 4));
    log("  Free blocks: %d (%d KB)", 1, 0,
        superblock.free_blocks, (superblock.free_blocks * 4));
    log("  Files: %d/%d", 1, 0, superblock.file_count, SFS_MAX_FILES);

    return SFS_OK;
}

sfs_error_t sfs_create(const char *filename, uint32_t size)
{
    if (!initialized)
    {
        return SFS_ERR_NOT_INITIALIZED;
    }

    if (!filename || strlen(filename) >= SFS_MAX_FILENAME)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    for (int i = 0; i < SFS_MAX_FILES; i++)
    {
        if (file_table[i].used && strcmp(file_table[i].filename, filename) == 0)
        {
            log("StartFS: File '%s' already exists", 2, 0, filename);
            return SFS_ERR_FILE_EXISTS;
        }
    }

    uint32_t blocks_needed = (size + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;
    if (blocks_needed == 0)
        blocks_needed = 1;

    if (blocks_needed > superblock.free_blocks)
    {
        log("StartFS: Not enough space (%d blocks needed, %d free)", 3, 1,
            blocks_needed, superblock.free_blocks);
        return SFS_ERR_NO_SPACE;
    }

    int free_entry = -1;
    for (int i = 0; i < SFS_MAX_FILES; i++)
    {
        if (!file_table[i].used)
        {
            free_entry = i;
            break;
        }
    }

    if (free_entry == -1)
    {
        log("StartFS: No free file entries (max %d files)", 3, 1, SFS_MAX_FILES);
        return SFS_ERR_TOO_MANY_FILES;
    }

    uint32_t start_block = 0;
    uint32_t found_blocks = 0;

    for (uint32_t block = 0; block < superblock.total_blocks; block++)
    {
        int is_free = 1;

        for (int i = 0; i < SFS_MAX_FILES; i++)
        {
            if (file_table[i].used)
            {
                uint32_t file_start = file_table[i].start_block;
                uint32_t file_end = file_start + file_table[i].block_count;

                if (block >= file_start && block < file_end)
                {
                    is_free = 0;
                    break;
                }
            }
        }

        if (is_free)
        {
            if (found_blocks == 0)
            {
                start_block = block;
            }
            found_blocks++;

            if (found_blocks >= blocks_needed)
            {
                break;
            }
        }
        else
        {
            found_blocks = 0;
        }
    }

    if (found_blocks < blocks_needed)
    {
        log("StartFS: Could not find contiguous space", 3, 1);
        return SFS_ERR_NO_SPACE;
    }

    strcpy(file_table[free_entry].filename, filename);
    file_table[free_entry].size = size;
    file_table[free_entry].start_block = start_block;
    file_table[free_entry].block_count = blocks_needed;
    file_table[free_entry].used = 1;

    superblock.file_count++;
    superblock.free_blocks -= blocks_needed;

    if (write_file_table() != SFS_OK)
    {
        return SFS_ERR_WRITE_FAILED;
    }

    if (write_superblock() != SFS_OK)
    {
        return SFS_ERR_WRITE_FAILED;
    }

    log("StartFS: Created '%s' (%d bytes, %d blocks at block %d)", 1, 0,
        filename, size, blocks_needed, start_block);

    return SFS_OK;
}

sfs_error_t sfs_open(const char *filename, sfs_file_t *file)
{
    if (!initialized)
    {
        return SFS_ERR_NOT_INITIALIZED;
    }

    if (!filename || !file)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    for (int i = 0; i < SFS_MAX_FILES; i++)
    {
        if (file_table[i].used && strcmp(file_table[i].filename, filename) == 0)
        {

            strcpy(file->filename, file_table[i].filename);
            file->size = file_table[i].size;
            file->start_block = file_table[i].start_block;
            file->position = 0;
            file->is_open = 1;

            log("StartFS: Opened '%s' (%d bytes)", 1, 0, filename, file->size);
            return SFS_OK;
        }
    }

    log("StartFS: File not found: '%s'", 2, 0, filename);
    return SFS_ERR_FILE_NOT_FOUND;
}

sfs_error_t sfs_read(sfs_file_t *file, void *buffer, uint32_t size, uint32_t *bytes_read)
{
    if (!initialized)
    {
        return SFS_ERR_NOT_INITIALIZED;
    }

    if (!file || !file->is_open)
    {
        return SFS_ERR_NOT_OPEN;
    }

    if (!buffer || size == 0)
    {
        if (bytes_read)
            *bytes_read = 0;
        return SFS_OK;
    }

    if (file->position >= file->size)
    {
        if (bytes_read)
            *bytes_read = 0;
        return SFS_ERR_EOF;
    }

    if (file->position + size > file->size)
    {
        size = file->size - file->position;
    }

    uint32_t total_read = 0;
    uint8_t *dest = (uint8_t *)buffer;
    uint8_t block_buffer[SFS_BLOCK_SIZE];

    while (total_read < size)
    {
        uint32_t current_block = file->start_block + (file->position / SFS_BLOCK_SIZE);
        uint32_t offset_in_block = file->position % SFS_BLOCK_SIZE;
        uint32_t bytes_to_read = SFS_BLOCK_SIZE - offset_in_block;

        if (bytes_to_read > size - total_read)
        {
            bytes_to_read = size - total_read;
        }

        uint32_t lba = block_to_lba(current_block);
        if (ata_read_sectors(superblock.drive_number, lba, 8, block_buffer) != ATA_SUCCESS)
        {
            if (bytes_read)
                *bytes_read = total_read;
            return SFS_ERR_READ_FAILED;
        }

        memcpy(dest + total_read, block_buffer + offset_in_block, bytes_to_read);

        total_read += bytes_to_read;
        file->position += bytes_to_read;
    }

    if (bytes_read)
    {
        *bytes_read = total_read;
    }

    return SFS_OK;
}

sfs_error_t sfs_write(sfs_file_t *file, const void *buffer, uint32_t size)
{
    if (!initialized)
    {
        return SFS_ERR_NOT_INITIALIZED;
    }

    if (!file || !file->is_open)
    {
        return SFS_ERR_NOT_OPEN;
    }

    if (!buffer || size == 0)
    {
        return SFS_OK;
    }

    if (file->position + size > file->size)
    {
        log("StartFS: Write would exceed file size", 3, 1);
        return SFS_ERR_INVALID_PARAM;
    }

    uint32_t total_written = 0;
    const uint8_t *src = (const uint8_t *)buffer;
    uint8_t block_buffer[SFS_BLOCK_SIZE];

    while (total_written < size)
    {
        uint32_t current_block = file->start_block + (file->position / SFS_BLOCK_SIZE);
        uint32_t offset_in_block = file->position % SFS_BLOCK_SIZE;
        uint32_t bytes_to_write = SFS_BLOCK_SIZE - offset_in_block;

        if (bytes_to_write > size - total_written)
        {
            bytes_to_write = size - total_written;
        }

        uint32_t lba = block_to_lba(current_block);

        if (offset_in_block != 0 || bytes_to_write < SFS_BLOCK_SIZE)
        {
            if (ata_read_sectors(superblock.drive_number, lba, 8, block_buffer) != ATA_SUCCESS)
            {
                return SFS_ERR_READ_FAILED;
            }
        }

        memcpy(block_buffer + offset_in_block, src + total_written, bytes_to_write);

        if (ata_write_sectors(superblock.drive_number, lba, 8, block_buffer) != ATA_SUCCESS)
        {
            return SFS_ERR_WRITE_FAILED;
        }

        total_written += bytes_to_write;
        file->position += bytes_to_write;
    }

    return SFS_OK;
}

sfs_error_t sfs_close(sfs_file_t *file)
{
    if (!file)
        return SFS_OK;

    file->is_open = 0;
    file->position = 0;

    return SFS_OK;
}

sfs_error_t sfs_delete(const char *filename)
{
    if (!initialized)
    {
        return SFS_ERR_NOT_INITIALIZED;
    }

    if (!filename)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    for (int i = 0; i < SFS_MAX_FILES; i++)
    {
        if (file_table[i].used && strcmp(file_table[i].filename, filename) == 0)
        {

            uint32_t freed_blocks = file_table[i].block_count;
            memset(&file_table[i], 0, sizeof(sfs_file_entry_t));

            superblock.file_count--;
            superblock.free_blocks += freed_blocks;

            if (write_file_table() != SFS_OK)
            {
                return SFS_ERR_WRITE_FAILED;
            }

            if (write_superblock() != SFS_OK)
            {
                return SFS_ERR_WRITE_FAILED;
            }

            log("StartFS: Deleted '%s'", 1, 0, filename);
            return SFS_OK;
        }
    }

    return SFS_ERR_FILE_NOT_FOUND;
}

sfs_error_t sfs_seek(sfs_file_t *file, uint32_t position)
{
    if (!file || !file->is_open)
    {
        return SFS_ERR_NOT_OPEN;
    }

    if (position > file->size)
    {
        position = file->size;
    }

    file->position = position;
    return SFS_OK;
}

sfs_error_t sfs_list_files(void)
{
    if (!initialized)
    {
        return SFS_ERR_NOT_INITIALIZED;
    }

    log("StartFS File Listing:", 1, 1);
    log("", 1, 1);

    int count = 0;
    for (int i = 0; i < SFS_MAX_FILES; i++)
    {
        if (file_table[i].used)
        {
            log("%s  %d bytes  (block %d)", 1, 1,
                file_table[i].filename,
                file_table[i].size,
                file_table[i].start_block);
            count++;
        }
    }

    if (count == 0)
    {
        log("(no files)", 1, 1);
    }

    log("", 1, 1);
    log("Total: %d files", 1, 1, count);

    return SFS_OK;
}

void sfs_print_stats(void)
{
    if (!initialized)
    {
        log("StartFS not initialized.", 2, 1);
        return;
    }

    log("StartFS Statistics:", 1, 1);
    log("  Total space: %d KB", 1, 1, superblock.total_blocks * 4);
    log("  Used space: %d KB", 1, 1,
        (superblock.total_blocks - superblock.free_blocks) * 4);
    log("  Free space: %d KB", 1, 1, superblock.free_blocks * 4);
    log("  Files: %d / %d", 1, 1, superblock.file_count, SFS_MAX_FILES);
}

sfs_error_t sfs_unmount(void)
{
    if (!initialized)
    {
        return SFS_ERR_NOT_INITIALIZED;
    }

    if (write_file_table() != SFS_OK)
    {
        return SFS_ERR_WRITE_FAILED;
    }

    if (write_superblock() != SFS_OK)
    {
        return SFS_ERR_WRITE_FAILED;
    }

    memset(&superblock, 0, sizeof(superblock));
    memset(file_table, 0, sizeof(file_table));

    initialized = 0;
    log("StartFS: Unmounted successfully", 4, 0);
    return SFS_OK;
}
