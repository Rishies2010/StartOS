#ifndef SFS_H
#define SFS_H

#include <stdint.h>
#include <stddef.h>

#define SFS_MAGIC           0x53465321      // "SFS!"
#define SFS_MAX_FILES       64
#define SFS_MAX_FILENAME    32
#define SFS_BLOCK_SIZE      4096            // 4KB blocks
#define SFS_SUPERBLOCK_LBA  0
#define SFS_FILETABLE_LBA   1
#define SFS_FILETABLE_SIZE  8               // 8 sectors for file table
#define SFS_DATA_START_LBA  9               // Data starts at sector 9

// Superblock structure (512 bytes, sector 0)
typedef struct {
    uint32_t magic;                 // Magic number "SFS!"
    uint32_t total_blocks;          // Total data blocks available
    uint32_t free_blocks;           // Free data blocks
    uint32_t file_count;            // Number of files
    uint8_t drive_number;           // Which drive this FS is on
    uint8_t reserved[495];          // Padding to 512 bytes
} __attribute__((packed)) sfs_superblock_t;

// File entry structure (64 bytes)
typedef struct {
    char filename[SFS_MAX_FILENAME];    // Filename (null terminated)
    uint32_t size;                      // File size in bytes
    uint32_t start_block;               // Starting block number
    uint32_t block_count;               // Number of blocks used
    uint8_t used;                       // 1 if used, 0 if free
    uint8_t reserved[23];               // Padding to 64 bytes
} __attribute__((packed)) sfs_file_entry_t;

// File handle for operations
typedef struct {
    char filename[SFS_MAX_FILENAME];
    uint32_t size;
    uint32_t start_block;
    uint32_t position;                  // Current read/write position
    uint8_t is_open;
} sfs_file_t;

// Error codes
typedef enum {
    SFS_OK = 0,
    SFS_ERR_NOT_INITIALIZED,
    SFS_ERR_INVALID_DRIVE,
    SFS_ERR_READ_FAILED,
    SFS_ERR_WRITE_FAILED,
    SFS_ERR_NOT_SFS,
    SFS_ERR_FILE_NOT_FOUND,
    SFS_ERR_NO_SPACE,
    SFS_ERR_FILE_EXISTS,
    SFS_ERR_TOO_MANY_FILES,
    SFS_ERR_NOT_OPEN,
    SFS_ERR_EOF,
    SFS_ERR_INVALID_PARAM
} sfs_error_t;

// Function declarations
sfs_error_t sfs_format(uint8_t drive);
sfs_error_t sfs_init(uint8_t drive);
sfs_error_t sfs_create(const char* filename, uint32_t size);
sfs_error_t sfs_open(const char* filename, sfs_file_t* file);
sfs_error_t sfs_read(sfs_file_t* file, void* buffer, uint32_t size, uint32_t* bytes_read);
sfs_error_t sfs_write(sfs_file_t* file, const void* buffer, uint32_t size);
sfs_error_t sfs_close(sfs_file_t* file);
sfs_error_t sfs_delete(const char* filename);
sfs_error_t sfs_list_files(void);
sfs_error_t sfs_seek(sfs_file_t* file, uint32_t position);
void sfs_print_stats(void);
sfs_error_t sfs_unmount(void);

#endif // SFS_H