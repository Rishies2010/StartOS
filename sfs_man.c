#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define SFS_MAGIC 0x53465321
#define SFS_BLOCK_SIZE 4096
#define SFS_MAX_FILES 64
#define SFS_MAX_FILENAME 32
#define SFS_SUPERBLOCK_LBA 0
#define SFS_FILETABLE_LBA 1
#define SFS_FILETABLE_SIZE 8
#define SFS_DATA_START_LBA 9

#define VHD_COOKIE 0x636f6e6563746978ULL

typedef struct {
    uint64_t cookie;
    uint32_t features;
    uint32_t file_format_version;
    uint64_t data_offset;
    uint32_t timestamp;
    uint8_t creator_application[4];
    uint32_t creator_version;
    uint32_t creator_host_os;
    uint64_t original_size;
    uint64_t current_size;
    uint32_t disk_geometry;
    uint32_t disk_type;
    uint32_t checksum;
    uint8_t unique_id[16];
    uint8_t saved_state;
    uint8_t reserved[427];
} __attribute__((packed)) vhd_footer_t;

typedef struct {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t file_count;
    uint8_t drive_number;
    uint8_t reserved[495];
} __attribute__((packed)) sfs_superblock_t;

typedef struct {
    char filename[SFS_MAX_FILENAME];
    uint32_t size;
    uint32_t start_block;
    uint32_t block_count;
    uint8_t used;
    uint8_t reserved[23];
} __attribute__((packed)) sfs_file_entry_t;

static int vhd_fd = -1;
static off_t vhd_data_offset = 0;
static sfs_superblock_t superblock;
static sfs_file_entry_t file_table[SFS_MAX_FILES];

uint32_t swap32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

uint64_t swap64(uint64_t val) {
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8) |
           ((val & 0x00000000FF000000ULL) << 8) |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
}

int detect_vhd_offset() {
    uint8_t sector[512];
    
    for (off_t offset = 0; offset < 65536; offset += 512) {
        if (lseek(vhd_fd, offset, SEEK_SET) < 0) {
            continue;
        }
        
        if (read(vhd_fd, sector, 512) != 512) {
            continue;
        }
        
        uint32_t *magic_ptr = (uint32_t*)sector;
        if (*magic_ptr == SFS_MAGIC) {
            vhd_data_offset = offset;
            printf("Found StartFS at offset 0x%lx (%ld bytes)\n", offset, offset);
            return 0;
        }
    }
    
    vhd_data_offset = 0;
    printf("No StartFS found, starting at offset 0 (use 'format' to initialize)\n");
    return 0;
}

int read_sector(uint32_t lba, void *buffer, uint32_t count) {
    off_t offset = vhd_data_offset + ((off_t)lba * 512);
    if (lseek(vhd_fd, offset, SEEK_SET) != offset) {
        return -1;
    }
    if (read(vhd_fd, buffer, count * 512) != (ssize_t)(count * 512)) {
        return -1;
    }
    return 0;
}

int write_sector(uint32_t lba, const void *buffer, uint32_t count) {
    off_t offset = vhd_data_offset + ((off_t)lba * 512);
    if (lseek(vhd_fd, offset, SEEK_SET) != offset) {
        return -1;
    }
    if (write(vhd_fd, buffer, count * 512) != (ssize_t)(count * 512)) {
        return -1;
    }
    return 0;
}

int load_filesystem() {
    if (read_sector(SFS_SUPERBLOCK_LBA, &superblock, 1) != 0) {
        fprintf(stderr, "Failed to read superblock\n");
        return -1;
    }
    
    if (superblock.magic != SFS_MAGIC) {
        fprintf(stderr, "Invalid StartFS magic (got 0x%x, expected 0x%x)\n", 
                superblock.magic, SFS_MAGIC);
        return -1;
    }
    
    if (read_sector(SFS_FILETABLE_LBA, file_table, SFS_FILETABLE_SIZE) != 0) {
        fprintf(stderr, "Failed to read file table\n");
        return -1;
    }
    
    return 0;
}

int save_filesystem() {
    if (write_sector(SFS_SUPERBLOCK_LBA, &superblock, 1) != 0) {
        fprintf(stderr, "Failed to write superblock\n");
        return -1;
    }
    
    if (write_sector(SFS_FILETABLE_LBA, file_table, SFS_FILETABLE_SIZE) != 0) {
        fprintf(stderr, "Failed to write file table\n");
        return -1;
    }
    
    return 0;
}

void cmd_format() {
    printf("Formatting StartFS...\n");
    
    memset(&superblock, 0, sizeof(sfs_superblock_t));
    superblock.magic = SFS_MAGIC;
    superblock.drive_number = 0;
    
    uint32_t total_sectors = 1048576;
    uint32_t data_sectors = total_sectors - SFS_DATA_START_LBA;
    superblock.total_blocks = data_sectors / 8;
    superblock.free_blocks = superblock.total_blocks;
    superblock.file_count = 0;
    
    memset(file_table, 0, sizeof(file_table));
    
    if (save_filesystem() != 0) {
        fprintf(stderr, "Format failed\n");
        return;
    }
    
    printf("Format complete! %d blocks (%d KB) available\n", 
           superblock.total_blocks, superblock.total_blocks * 4);
}

void cmd_info() {
    printf("\nStartFS Statistics:\n");
    printf("  Total space: %d KB\n", superblock.total_blocks * 4);
    printf("  Used space:  %d KB\n", 
           (superblock.total_blocks - superblock.free_blocks) * 4);
    printf("  Free space:  %d KB\n", superblock.free_blocks * 4);
    printf("  Files:       %d / %d\n\n", superblock.file_count, SFS_MAX_FILES);
}

void cmd_list() {
    printf("\nFiles in StartFS:\n");
    printf("%-56s %10s %10s\n", "Filename", "Size", "Block");
    printf("--------------------------------------------------------------------------------\n");
    
    int count = 0;
    for (int i = 0; i < SFS_MAX_FILES; i++) {
        if (file_table[i].used) {
            printf("%-56s %10u %10u\n", 
                   file_table[i].filename,
                   file_table[i].size,
                   file_table[i].start_block);
            count++;
        }
    }
    
    if (count == 0) {
        printf("(no files)\n");
    }
    printf("\nTotal: %d files\n\n", count);
}

int find_file(const char *filename) {
    for (int i = 0; i < SFS_MAX_FILES; i++) {
        if (file_table[i].used && strcmp(file_table[i].filename, filename) == 0) {
            return i;
        }
    }
    return -1;
}

uint32_t find_free_space(uint32_t blocks_needed) {
    uint32_t start_block = 0;
    uint32_t found_blocks = 0;
    
    for (uint32_t block = 0; block < superblock.total_blocks; block++) {
        int is_free = 1;
        
        for (int i = 0; i < SFS_MAX_FILES; i++) {
            if (file_table[i].used) {
                uint32_t file_start = file_table[i].start_block;
                uint32_t file_end = file_start + file_table[i].block_count;
                
                if (block >= file_start && block < file_end) {
                    is_free = 0;
                    break;
                }
            }
        }
        
        if (is_free) {
            if (found_blocks == 0) {
                start_block = block;
            }
            found_blocks++;
            
            if (found_blocks >= blocks_needed) {
                return start_block;
            }
        } else {
            found_blocks = 0;
        }
    }
    
    return 0xFFFFFFFF;
}

void cmd_import(const char *host_path, const char *sfs_name) {
    FILE *f = fopen(host_path, "rb");
    if (!f) {
        fprintf(stderr, "Cannot open host file: %s\n", host_path);
        return;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (strlen(sfs_name) >= SFS_MAX_FILENAME) {
        fprintf(stderr, "Filename too long (max %d)\n", SFS_MAX_FILENAME - 1);
        fclose(f);
        return;
    }
    
    if (find_file(sfs_name) != -1) {
        fprintf(stderr, "File already exists: %s\n", sfs_name);
        fclose(f);
        return;
    }
    
    uint32_t blocks_needed = (size + SFS_BLOCK_SIZE - 1) / SFS_BLOCK_SIZE;
    if (blocks_needed == 0) blocks_needed = 1;
    
    if (blocks_needed > superblock.free_blocks) {
        fprintf(stderr, "Not enough space (%d blocks needed, %d free)\n",
                blocks_needed, superblock.free_blocks);
        fclose(f);
        return;
    }
    
    int free_entry = -1;
    for (int i = 0; i < SFS_MAX_FILES; i++) {
        if (!file_table[i].used) {
            free_entry = i;
            break;
        }
    }
    
    if (free_entry == -1) {
        fprintf(stderr, "Too many files (max %d)\n", SFS_MAX_FILES);
        fclose(f);
        return;
    }
    
    uint32_t start_block = find_free_space(blocks_needed);
    if (start_block == 0xFFFFFFFF) {
        fprintf(stderr, "Could not find contiguous space\n");
        fclose(f);
        return;
    }
    
    uint8_t *block_buffer = malloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(f);
        return;
    }
    
    for (uint32_t i = 0; i < blocks_needed; i++) {
        memset(block_buffer, 0, SFS_BLOCK_SIZE);
        size_t bytes_read = fread(block_buffer, 1, SFS_BLOCK_SIZE, f);
        
        uint32_t lba = SFS_DATA_START_LBA + ((start_block + i) * 8);
        if (write_sector(lba, block_buffer, 8) != 0) {
            fprintf(stderr, "Write failed at block %d\n", i);
            free(block_buffer);
            fclose(f);
            return;
        }
    }
    
    free(block_buffer);
    fclose(f);
    
    strcpy(file_table[free_entry].filename, sfs_name);
    file_table[free_entry].size = size;
    file_table[free_entry].start_block = start_block;
    file_table[free_entry].block_count = blocks_needed;
    file_table[free_entry].used = 1;
    
    superblock.file_count++;
    superblock.free_blocks -= blocks_needed;
    
    if (save_filesystem() != 0) {
        fprintf(stderr, "Failed to update filesystem\n");
        return;
    }
    
    printf("Imported: %s (%ld bytes, %d blocks)\n", sfs_name, size, blocks_needed);
}

void cmd_export(const char *sfs_name, const char *host_path) {
    int idx = find_file(sfs_name);
    if (idx == -1) {
        fprintf(stderr, "File not found: %s\n", sfs_name);
        return;
    }
    
    FILE *f = fopen(host_path, "wb");
    if (!f) {
        fprintf(stderr, "Cannot create host file: %s\n", host_path);
        return;
    }
    
    sfs_file_entry_t *entry = &file_table[idx];
    uint8_t *block_buffer = malloc(SFS_BLOCK_SIZE);
    if (!block_buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        fclose(f);
        return;
    }
    
    uint32_t bytes_left = entry->size;
    
    for (uint32_t i = 0; i < entry->block_count; i++) {
        uint32_t lba = SFS_DATA_START_LBA + ((entry->start_block + i) * 8);
        if (read_sector(lba, block_buffer, 8) != 0) {
            fprintf(stderr, "Read failed at block %d\n", i);
            free(block_buffer);
            fclose(f);
            return;
        }
        
        uint32_t to_write = (bytes_left > SFS_BLOCK_SIZE) ? SFS_BLOCK_SIZE : bytes_left;
        fwrite(block_buffer, 1, to_write, f);
        bytes_left -= to_write;
    }
    
    free(block_buffer);
    fclose(f);
    
    printf("Exported: %s -> %s (%u bytes)\n", sfs_name, host_path, entry->size);
}

void cmd_delete(const char *sfs_name) {
    int idx = find_file(sfs_name);
    if (idx == -1) {
        fprintf(stderr, "File not found: %s\n", sfs_name);
        return;
    }
    
    uint32_t freed_blocks = file_table[idx].block_count;
    memset(&file_table[idx], 0, sizeof(sfs_file_entry_t));
    
    superblock.file_count--;
    superblock.free_blocks += freed_blocks;
    
    if (save_filesystem() != 0) {
        fprintf(stderr, "Failed to update filesystem\n");
        return;
    }
    
    printf("Deleted: %s (%d blocks freed)\n", sfs_name, freed_blocks);
}

void show_help() {
    printf("StartFS Manager - Manage StartFS filesystems from Linux\n\n");
    printf("Usage: sfs_man <vhd_file> <command> [args]\n\n");
    printf("Commands:\n");
    printf("  format              Format the VHD with StartFS\n");
    printf("  info                Show filesystem statistics\n");
    printf("  list                List all files\n");
    printf("  import <src> <dst>  Import file from host to StartFS\n");
    printf("  export <src> <dst>  Export file from StartFS to host\n");
    printf("  delete <file>       Delete file from StartFS\n");
    printf("  help                Show this help\n\n");
    printf("Examples:\n");
    printf("  sfs_man StartOS.vhd format\n");
    printf("  sfs_man StartOS.vhd import build/kernel.bin kernel\n");
    printf("  sfs_man StartOS.vhd list\n");
    printf("  sfs_man StartOS.vhd export kernel kernel_backup.bin\n");
}

int main(int argc, char **argv) {
    if (argc < 3) {
        show_help();
        return 1;
    }
    
    const char *vhd_path = argv[1];
    const char *command = argv[2];
    
    if (strcmp(command, "help") == 0) {
        show_help();
        return 0;
    }
    
    vhd_fd = open(vhd_path, O_RDWR);
    if (vhd_fd < 0) {
        fprintf(stderr, "Cannot open VHD file: %s\n", vhd_path);
        return 1;
    }
    
    if (detect_vhd_offset() != 0) {
        fprintf(stderr, "Failed to detect VHD format\n");
        close(vhd_fd);
        return 1;
    }
    
    if (strcmp(command, "format") == 0) {
        cmd_format();
    } else {
        if (load_filesystem() != 0) {
            close(vhd_fd);
            return 1;
        }
        
        if (strcmp(command, "info") == 0) {
            cmd_info();
        } else if (strcmp(command, "list") == 0) {
            cmd_list();
        } else if (strcmp(command, "import") == 0) {
            if (argc != 5) {
                fprintf(stderr, "Usage: import <host_file> <sfs_name>\n");
            } else {
                cmd_import(argv[3], argv[4]);
            }
        } else if (strcmp(command, "export") == 0) {
            if (argc != 5) {
                fprintf(stderr, "Usage: export <sfs_name> <host_file>\n");
            } else {
                cmd_export(argv[3], argv[4]);
            }
        } else if (strcmp(command, "delete") == 0) {
            if (argc != 4) {
                fprintf(stderr, "Usage: delete <sfs_name>\n");
            } else {
                cmd_delete(argv[3]);
            }
        } else {
            fprintf(stderr, "Unknown command: %s\n", command);
            show_help();
        }
    }
    
    close(vhd_fd);
    return 0;
}