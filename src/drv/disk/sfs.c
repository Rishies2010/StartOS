#include "sfs.h"
#include "../disk/ata.h"
#include "../../libk/core/mem.h"
#include "../../libk/string.h"
#include "../../libk/debug/log.h"

static sfs_superblock_t superblock;
static sfs_entry_t entry_table[SFS_MAX_ENTRIES];
static uint8_t initialized = 0;
static uint8_t current_dir = SFS_ROOT_DIR_INDEX;

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

static sfs_error_t write_entry_table(void)
{
    if (ata_write_sectors(superblock.drive_number, SFS_FILETABLE_LBA,
                          SFS_FILETABLE_SIZE, entry_table) != ATA_SUCCESS)
    {
        return SFS_ERR_WRITE_FAILED;
    }
    return SFS_OK;
}

static int find_entry(const char *name, uint8_t parent, uint8_t type)
{
    for (int i = 0; i < SFS_MAX_ENTRIES; i++)
    {
        if (entry_table[i].type == type &&
            entry_table[i].parent_index == parent &&
            strcmp(entry_table[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

static int find_free_entry(void)
{
    for (int i = 0; i < SFS_MAX_ENTRIES; i++)
    {
        if (entry_table[i].type == SFS_TYPE_UNUSED)
        {
            return i;
        }
    }
    return -1;
}

static int resolve_path(const char *path, uint8_t *out_parent, char *out_name)
{
    if (!path || !out_parent || !out_name)
        return -1;

    if (path[0] == '/')
    {
        *out_parent = SFS_ROOT_DIR_INDEX;
        path++;
    }
    else
    {
        *out_parent = current_dir;
    }

    if (strlen(path) == 0)
    {
        out_name[0] = '\0';
        return 0;
    }

    char temp[SFS_MAX_FILENAME];
    int idx = 0;
    uint8_t current = *out_parent;

    for (size_t i = 0; i <= strlen(path); i++)
    {
        if (path[i] == '/' || path[i] == '\0')
        {
            if (idx > 0)
            {
                temp[idx] = '\0';

                if (strcmp(temp, "..") == 0)
                {
                    if (current != SFS_ROOT_DIR_INDEX)
                    {
                        current = entry_table[current].parent_index;
                    }
                }
                else if (strcmp(temp, ".") != 0)
                {
                    if (path[i] == '\0')
                    {
                        strcpy(out_name, temp);
                        *out_parent = current;
                        return 0;
                    }
                    else
                    {
                        int dir_idx = find_entry(temp, current, SFS_TYPE_DIRECTORY);
                        if (dir_idx < 0)
                            return -1;
                        current = dir_idx;
                    }
                }

                idx = 0;
            }
        }
        else
        {
            if (idx < SFS_MAX_FILENAME - 1)
            {
                temp[idx++] = path[i];
            }
        }
    }

    out_name[0] = '\0';
    *out_parent = current;
    return 0;
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
    superblock.entry_count = 1;

    memset(entry_table, 0, sizeof(entry_table));

    strcpy(entry_table[0].name, "/");
    entry_table[0].type = SFS_TYPE_DIRECTORY;
    entry_table[0].parent_index = 0;

    if (write_superblock() != SFS_OK)
    {
        log("StartFS: Failed to write superblock", 3, 1);
        return SFS_ERR_WRITE_FAILED;
    }

    if (write_entry_table() != SFS_OK)
    {
        log("StartFS: Failed to write entry table", 3, 1);
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
                         entry_table) != ATA_SUCCESS)
    {
        log("StartFS: Failed to read entry table", 3, 1);
        return SFS_ERR_READ_FAILED;
    }

    initialized = 1;
    current_dir = SFS_ROOT_DIR_INDEX;

    log("StartFS: Initialized successfully", 4, 0);
    log("  Total blocks: %d (%d KB)", 1, 0,
        superblock.total_blocks, (superblock.total_blocks * 4));
    log("  Free blocks: %d (%d KB)", 1, 0,
        superblock.free_blocks, (superblock.free_blocks * 4));
    log("  Entries: %d/%d", 1, 0, superblock.entry_count, SFS_MAX_ENTRIES);

    return SFS_OK;
}

sfs_error_t sfs_mkdir(const char *dirname)
{
    if (!initialized)
        return SFS_ERR_NOT_INITIALIZED;

    if (!dirname || strlen(dirname) == 0)
        return SFS_ERR_INVALID_PARAM;

    uint8_t parent;
    char name[SFS_MAX_FILENAME];

    if (resolve_path(dirname, &parent, name) < 0 || strlen(name) == 0)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (find_entry(name, parent, SFS_TYPE_DIRECTORY) >= 0 ||
        find_entry(name, parent, SFS_TYPE_FILE) >= 0)
    {
        log("StartFS: '%s' already exists", 2, 0, name);
        return SFS_ERR_ALREADY_EXISTS;
    }

    int free_idx = find_free_entry();
    if (free_idx < 0)
    {
        log("StartFS: No free entries", 3, 1);
        return SFS_ERR_TOO_MANY_ENTRIES;
    }

    strcpy(entry_table[free_idx].name, name);
    entry_table[free_idx].type = SFS_TYPE_DIRECTORY;
    entry_table[free_idx].parent_index = parent;
    entry_table[free_idx].size = 0;
    entry_table[free_idx].start_block = 0;
    entry_table[free_idx].block_count = 0;

    superblock.entry_count++;

    if (write_entry_table() != SFS_OK)
        return SFS_ERR_WRITE_FAILED;

    if (write_superblock() != SFS_OK)
        return SFS_ERR_WRITE_FAILED;

    log("StartFS: Created directory '%s'", 1, 0, name);
    return SFS_OK;
}

sfs_error_t sfs_rmdir(const char *dirname)
{
    if (!initialized)
        return SFS_ERR_NOT_INITIALIZED;

    if (!dirname)
        return SFS_ERR_INVALID_PARAM;

    uint8_t parent;
    char name[SFS_MAX_FILENAME];

    if (resolve_path(dirname, &parent, name) < 0 || strlen(name) == 0)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    int dir_idx = find_entry(name, parent, SFS_TYPE_DIRECTORY);
    if (dir_idx < 0)
    {
        return SFS_ERR_FILE_NOT_FOUND;
    }

    if (dir_idx == SFS_ROOT_DIR_INDEX)
    {
        log("StartFS: Cannot remove root directory", 3, 1);
        return SFS_ERR_INVALID_PARAM;
    }

    for (int i = 0; i < SFS_MAX_ENTRIES; i++)
    {
        if (entry_table[i].type != SFS_TYPE_UNUSED &&
            entry_table[i].parent_index == dir_idx)
        {
            log("StartFS: Directory not empty", 3, 1);
            return SFS_ERR_NOT_EMPTY;
        }
    }

    memset(&entry_table[dir_idx], 0, sizeof(sfs_entry_t));
    superblock.entry_count--;

    if (write_entry_table() != SFS_OK)
        return SFS_ERR_WRITE_FAILED;

    if (write_superblock() != SFS_OK)
        return SFS_ERR_WRITE_FAILED;

    log("StartFS: Removed directory '%s'", 1, 0, name);
    return SFS_OK;
}

sfs_error_t sfs_chdir(const char *dirname)
{
    if (!initialized)
        return SFS_ERR_NOT_INITIALIZED;

    if (!dirname)
        return SFS_ERR_INVALID_PARAM;

    if (strcmp(dirname, "/") == 0)
    {
        current_dir = SFS_ROOT_DIR_INDEX;
        return SFS_OK;
    }

    uint8_t parent;
    char name[SFS_MAX_FILENAME];

    if (resolve_path(dirname, &parent, name) < 0)
    {
        return SFS_ERR_FILE_NOT_FOUND;
    }

    if (strlen(name) == 0)
    {
        current_dir = parent;
        return SFS_OK;
    }

    int dir_idx = find_entry(name, parent, SFS_TYPE_DIRECTORY);
    if (dir_idx < 0)
    {
        return SFS_ERR_FILE_NOT_FOUND;
    }

    current_dir = dir_idx;
    return SFS_OK;
}

void sfs_get_cwd(char *buffer, size_t size)
{
    if (!initialized || !buffer || size == 0)
    {
        if (buffer && size > 0)
            buffer[0] = '\0';
        return;
    }

    if (current_dir == SFS_ROOT_DIR_INDEX)
    {
        if (size >= 2)
        {
            buffer[0] = '/';
            buffer[1] = '\0';
        }
        return;
    }

    int depth = 0;
    uint8_t idx = current_dir;
    char *parts[16];

    while (idx != SFS_ROOT_DIR_INDEX && depth < 16)
    {
        parts[depth++] = entry_table[idx].name;
        idx = entry_table[idx].parent_index;
    }

    buffer[0] = '\0';
    for (int i = depth - 1; i >= 0; i--)
    {
        strcat(buffer, "/");
        strcat(buffer, parts[i]);
    }

    if (buffer[0] == '\0')
    {
        buffer[0] = '/';
        buffer[1] = '\0';
    }
}

sfs_error_t sfs_create(const char *filename, uint32_t size)
{
    if (!initialized)
        return SFS_ERR_NOT_INITIALIZED;

    if (!filename || strlen(filename) == 0)
        return SFS_ERR_INVALID_PARAM;

    uint8_t parent;
    char name[SFS_MAX_FILENAME];

    if (resolve_path(filename, &parent, name) < 0 || strlen(name) == 0)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (find_entry(name, parent, SFS_TYPE_FILE) >= 0 ||
        find_entry(name, parent, SFS_TYPE_DIRECTORY) >= 0)
    {
        log("StartFS: '%s' already exists", 2, 0, name);
        return SFS_ERR_ALREADY_EXISTS;
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

    int free_idx = find_free_entry();
    if (free_idx < 0)
    {
        log("StartFS: No free entries", 3, 1);
        return SFS_ERR_TOO_MANY_ENTRIES;
    }

    uint32_t start_block = 0;
    uint32_t found_blocks = 0;

    for (uint32_t block = 0; block < superblock.total_blocks; block++)
    {
        int is_free = 1;

        for (int i = 0; i < SFS_MAX_ENTRIES; i++)
        {
            if (entry_table[i].type == SFS_TYPE_FILE)
            {
                uint32_t file_start = entry_table[i].start_block;
                uint32_t file_end = file_start + entry_table[i].block_count;

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

    strcpy(entry_table[free_idx].name, name);
    entry_table[free_idx].size = size;
    entry_table[free_idx].start_block = start_block;
    entry_table[free_idx].block_count = blocks_needed;
    entry_table[free_idx].type = SFS_TYPE_FILE;
    entry_table[free_idx].parent_index = parent;

    superblock.entry_count++;
    superblock.free_blocks -= blocks_needed;

    if (write_entry_table() != SFS_OK)
        return SFS_ERR_WRITE_FAILED;

    if (write_superblock() != SFS_OK)
        return SFS_ERR_WRITE_FAILED;

    log("StartFS: Created '%s' (%d bytes, %d blocks)", 1, 0,
        name, size, blocks_needed);

    return SFS_OK;
}

sfs_error_t sfs_open(const char *filename, sfs_file_t *file)
{
    if (!initialized)
        return SFS_ERR_NOT_INITIALIZED;

    if (!filename || !file)
        return SFS_ERR_INVALID_PARAM;

    uint8_t parent;
    char name[SFS_MAX_FILENAME];

    if (resolve_path(filename, &parent, name) < 0 || strlen(name) == 0)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    int file_idx = find_entry(name, parent, SFS_TYPE_FILE);
    if (file_idx < 0)
    {
        log("StartFS: File not found: '%s'", 2, 0, name);
        return SFS_ERR_FILE_NOT_FOUND;
    }

    strcpy(file->name, entry_table[file_idx].name);
    file->size = entry_table[file_idx].size;
    file->start_block = entry_table[file_idx].start_block;
    file->position = 0;
    file->entry_index = file_idx;
    file->is_open = 1;

    log("StartFS: Opened '%s' (%d bytes)", 1, 0, name, file->size);
    return SFS_OK;
}

sfs_error_t sfs_read(sfs_file_t *file, void *buffer, uint32_t size, uint32_t *bytes_read)
{
    if (!initialized)
        return SFS_ERR_NOT_INITIALIZED;

    if (!file || !file->is_open)
        return SFS_ERR_NOT_OPEN;

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
        return SFS_ERR_NOT_INITIALIZED;

    if (!file || !file->is_open)
        return SFS_ERR_NOT_OPEN;

    if (!buffer || size == 0)
        return SFS_OK;

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
        return SFS_ERR_NOT_INITIALIZED;

    if (!filename)
        return SFS_ERR_INVALID_PARAM;

    uint8_t parent;
    char name[SFS_MAX_FILENAME];

    if (resolve_path(filename, &parent, name) < 0 || strlen(name) == 0)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    int file_idx = find_entry(name, parent, SFS_TYPE_FILE);
    if (file_idx < 0)
    {
        return SFS_ERR_FILE_NOT_FOUND;
    }

    uint32_t freed_blocks = entry_table[file_idx].block_count;
    memset(&entry_table[file_idx], 0, sizeof(sfs_entry_t));

    superblock.entry_count--;
    superblock.free_blocks += freed_blocks;

    if (write_entry_table() != SFS_OK)
        return SFS_ERR_WRITE_FAILED;

    if (write_superblock() != SFS_OK)
        return SFS_ERR_WRITE_FAILED;

    log("StartFS: Deleted '%s'", 1, 0, name);
    return SFS_OK;
}

sfs_error_t sfs_seek(sfs_file_t *file, uint32_t position)
{
    if (!file || !file->is_open)
        return SFS_ERR_NOT_OPEN;

    if (position > file->size)
    {
        position = file->size;
    }

    file->position = position;
    return SFS_OK;
}

sfs_error_t sfs_list(void)
{
    if (!initialized)
        return SFS_ERR_NOT_INITIALIZED;

    char cwd[256];
    sfs_get_cwd(cwd, sizeof(cwd));
    log("Directory listing for: %s", 1, 1, cwd);
    log("", 1, 1);

    int file_count = 0;
    int dir_count = 0;

    for (int i = 0; i < SFS_MAX_ENTRIES; i++)
    {
        if (entry_table[i].type != SFS_TYPE_UNUSED &&
            entry_table[i].parent_index == current_dir)
        {
            if (entry_table[i].type == SFS_TYPE_DIRECTORY)
            {
                log("[DIR]  %s", 1, 1, entry_table[i].name);
                dir_count++;
            }
            else if (entry_table[i].type == SFS_TYPE_FILE)
            {
                log("       %s  (%d bytes)", 1, 1,
                    entry_table[i].name,
                    entry_table[i].size);
                file_count++;
            }
        }
    }

    if (file_count == 0 && dir_count == 0)
    {
        log("(empty)", 1, 1);
    }

    log("", 1, 1);
    log("Total: %d directories, %d files", 1, 1, dir_count, file_count);

    return SFS_OK;
}

void sfs_print_stats(void)
{
    if (!initialized)
    {
        log("StartFS not initialized.", 2, 1);
        return;
    }

    int file_count = 0;
    int dir_count = 0;

    for (int i = 0; i < SFS_MAX_ENTRIES; i++)
    {
        if (entry_table[i].type == SFS_TYPE_FILE)
            file_count++;
        else if (entry_table[i].type == SFS_TYPE_DIRECTORY)
            dir_count++;
    }

    log("StartFS Statistics:", 1, 1);
    log("  Total space: %d KB", 1, 1, superblock.total_blocks * 4);
    log("  Used space: %d KB", 1, 1,
        (superblock.total_blocks - superblock.free_blocks) * 4);
    log("  Free space: %d KB", 1, 1, superblock.free_blocks * 4);
    log("  Entries: %d / %d", 1, 1, superblock.entry_count, SFS_MAX_ENTRIES);
    log("  Directories: %d", 1, 1, dir_count);
    log("  Files: %d", 1, 1, file_count);
}

sfs_error_t sfs_unmount(void)
{
    if (!initialized)
    {
        return SFS_ERR_NOT_INITIALIZED;
    }

    if (write_entry_table() != SFS_OK)
    {
        return SFS_ERR_WRITE_FAILED;
    }

    if (write_superblock() != SFS_OK)
    {
        return SFS_ERR_WRITE_FAILED;
    }

    memset(&superblock, 0, sizeof(superblock));
    memset(entry_table, 0, sizeof(entry_table));

    initialized = 0;
    current_dir = SFS_ROOT_DIR_INDEX;
    log("StartFS: Unmounted successfully", 4, 0);
    return SFS_OK;
}