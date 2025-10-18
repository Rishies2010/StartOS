#include "wrap.h"
#include "socket.h"
#include "../string.h"
#include "mem.h"

typedef union
{
    sfs_file_t sfs;
    socket_file_t *sock;
} fs_handle_t;

sfs_error_t fs_open(bool fs_select, const char *filename, void **handle)
{
    if (!filename || !handle)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (fs_select)
    {

        sfs_file_t *sfs_file = (sfs_file_t *)kmalloc(sizeof(sfs_file_t));
        if (!sfs_file)
            return SFS_ERR_NO_SPACE;

        sfs_error_t err = sfs_open(filename, sfs_file);
        if (err != SFS_OK)
        {
            kfree(sfs_file);
            return err;
        }
        *handle = sfs_file;
        return SFS_OK;
    }
    else
    {

        socket_file_t *sock;
        socket_error_t err = socket_open(filename, &sock);
        *handle = sock;
        return (sfs_error_t)err;
    }
}

sfs_error_t fs_read(bool fs_select, void *handle, void *buffer, uint32_t size, uint32_t *bytes_read)
{
    if (!handle || !buffer || !bytes_read)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (fs_select)
    {

        return sfs_read((sfs_file_t *)handle, buffer, size, bytes_read);
    }
    else
    {

        return (sfs_error_t)socket_read((socket_file_t *)handle, buffer, size, bytes_read);
    }
}

sfs_error_t fs_write(bool fs_select, void *handle, const void *buffer, uint32_t size)
{
    if (!handle || !buffer)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (fs_select)
    {

        return sfs_write((sfs_file_t *)handle, buffer, size);
    }
    else
    {

        return (sfs_error_t)socket_write((socket_file_t *)handle, buffer, size);
    }
}

sfs_error_t fs_close(bool fs_select, void *handle)
{
    if (!handle)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (fs_select)
    {

        sfs_error_t err = sfs_close((sfs_file_t *)handle);
        kfree(handle);
        return err;
    }
    else
    {

        return (sfs_error_t)socket_close((socket_file_t *)handle);
    }
}

sfs_error_t fs_create(bool fs_select, const char *filename, uint32_t size)
{
    if (!filename)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (fs_select)
    {

        return sfs_create(filename, size);
    }
    else
    {

        return (sfs_error_t)socket_create(filename);
    }
}

sfs_error_t fs_delete(bool fs_select, const char *filename)
{
    if (!filename)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (fs_select)
    {

        return sfs_delete(filename);
    }
    else
    {

        return (sfs_error_t)socket_delete(filename);
    }
}

sfs_error_t fs_seek(bool fs_select, void *handle, uint32_t position)
{
    if (!handle)
    {
        return SFS_ERR_INVALID_PARAM;
    }

    if (fs_select)
    {

        return sfs_seek((sfs_file_t *)handle, position);
    }
    else
    {

        return SFS_ERR_INVALID_PARAM;
    }
}

sfs_error_t fs_format(uint8_t drive)
{
    return sfs_format(drive);
}

sfs_error_t fs_init(uint8_t drive)
{
    return sfs_init(drive);
}

sfs_error_t fs_mkdir(const char *dirname)
{
    return sfs_mkdir(dirname);
}

sfs_error_t fs_rmdir(const char *dirname)
{
    return sfs_rmdir(dirname);
}

sfs_error_t fs_chdir(const char *dirname)
{
    return sfs_chdir(dirname);
}

void fs_get_cwd(char *buffer, size_t size)
{
    sfs_get_cwd(buffer, size);
}

sfs_error_t fs_list(void)
{
    return sfs_list();
}

void fs_print_stats(void)
{
    sfs_print_stats();
}

sfs_error_t fs_unmount(void)
{
    return sfs_unmount();
}