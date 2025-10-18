#ifndef FS_WRAPPER_H
#define FS_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../drv/disk/sfs.h"

sfs_error_t fs_open(bool fs_select, const char *filename, void **handle);
sfs_error_t fs_read(bool fs_select, void *handle, void *buffer, uint32_t size, uint32_t *bytes_read);
sfs_error_t fs_write(bool fs_select, void *handle, const void *buffer, uint32_t size);
sfs_error_t fs_close(bool fs_select, void *handle);
sfs_error_t fs_create(bool fs_select, const char *filename, uint32_t size);
sfs_error_t fs_delete(bool fs_select, const char *filename);
sfs_error_t fs_seek(bool fs_select, void *handle, uint32_t position);
sfs_error_t fs_format(uint8_t drive);
sfs_error_t fs_init(uint8_t drive);
sfs_error_t fs_mkdir(const char *dirname);
sfs_error_t fs_rmdir(const char *dirname);
sfs_error_t fs_chdir(const char *dirname);
void fs_get_cwd(char *buffer, size_t size);
sfs_error_t fs_list(void);
void fs_print_stats(void);
sfs_error_t fs_unmount(void);

#endif