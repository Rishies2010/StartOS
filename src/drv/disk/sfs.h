#ifndef SFS_H
#define SFS_H

#include <stdint.h>
#include <stddef.h>

#define SFS_MAGIC           0x53465321      
#define SFS_MAX_ENTRIES     64              
#define SFS_MAX_FILENAME    28              
#define SFS_BLOCK_SIZE      4096            
#define SFS_SUPERBLOCK_LBA  0
#define SFS_FILETABLE_LBA   1
#define SFS_FILETABLE_SIZE  8               
#define SFS_DATA_START_LBA  9               

#define SFS_ROOT_DIR_INDEX  0               


#define SFS_TYPE_UNUSED     0
#define SFS_TYPE_FILE       1
#define SFS_TYPE_DIRECTORY  2


typedef struct {
    uint32_t magic;                 
    uint32_t total_blocks;          
    uint32_t free_blocks;           
    uint32_t entry_count;           
    uint8_t drive_number;           
    uint8_t reserved[495];          
} __attribute__((packed)) sfs_superblock_t;


typedef struct {
    char name[SFS_MAX_FILENAME];        
    uint32_t size;                      
    uint32_t start_block;               
    uint32_t block_count;               
    uint8_t type;                       
    uint8_t parent_index;               
    uint8_t reserved[26];               
} __attribute__((packed)) sfs_entry_t;


typedef struct {
    char name[SFS_MAX_FILENAME];
    uint32_t size;
    uint32_t start_block;
    uint32_t position;                  
    uint8_t entry_index;                
    uint8_t is_open;
} sfs_file_t;


typedef enum {
    SFS_OK = 0,
    SFS_ERR_NOT_INITIALIZED,
    SFS_ERR_INVALID_DRIVE,
    SFS_ERR_READ_FAILED,
    SFS_ERR_WRITE_FAILED,
    SFS_ERR_NOT_SFS,
    SFS_ERR_FILE_NOT_FOUND,
    SFS_ERR_NO_SPACE,
    SFS_ERR_ALREADY_EXISTS,
    SFS_ERR_TOO_MANY_ENTRIES,
    SFS_ERR_NOT_OPEN,
    SFS_ERR_EOF,
    SFS_ERR_INVALID_PARAM,
    SFS_ERR_NOT_EMPTY,
    SFS_ERR_NOT_A_DIRECTORY,
    SFS_ERR_IS_DIRECTORY
} sfs_error_t;


sfs_error_t sfs_format(uint8_t drive);
sfs_error_t sfs_init(uint8_t drive);


sfs_error_t sfs_mkdir(const char* dirname);
sfs_error_t sfs_rmdir(const char* dirname);
sfs_error_t sfs_chdir(const char* dirname);
void sfs_get_cwd(char* buffer, size_t size);


sfs_error_t sfs_create(const char* filename, uint32_t size);
sfs_error_t sfs_open(const char* filename, sfs_file_t* file);
sfs_error_t sfs_read(sfs_file_t* file, void* buffer, uint32_t size, uint32_t* bytes_read);
sfs_error_t sfs_write(sfs_file_t* file, const void* buffer, uint32_t size);
sfs_error_t sfs_close(sfs_file_t* file);
sfs_error_t sfs_delete(const char* filename);
sfs_error_t sfs_seek(sfs_file_t* file, uint32_t position);


sfs_error_t sfs_list(void);  
void sfs_print_stats(void);


sfs_error_t sfs_unmount(void);

#endif 