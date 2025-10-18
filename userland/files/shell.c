#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "../kernel_api.h"

static kernel_api_t *g_api = NULL;
static char command_buffer[256];

static int parse_command(char *command, char *args[])
{
    int arg_count = 0;
    bool in_arg = false;
    while (*command && arg_count < 32)
    {
        if (*command == ' ' || *command == '\t')
        {
            *command = '\0';
            in_arg = false;
        }
        else if (!in_arg)
        {
            args[arg_count++] = command;
            in_arg = true;
        }

        command++;
    }

    return arg_count;
}

static void cmd_version(void)
{
    prints("\n  StartOS 0.90.0 UNSTABLE\n  By rishiewas2010\n");
}

static void cmd_date(void)
{
    rtc_time_t current_time = rtc_get_time();
    prints(" Current Date: ");
    printc('0' + (current_time.day / 10));
    printc('0' + (current_time.day % 10));
    printc('/');
    printc('0' + (current_time.month / 10));
    printc('0' + (current_time.month % 10));
    printc('/');
    prints("20");
    printc('0' + (current_time.year / 10));
    printc('0' + (current_time.year % 10));
    prints("\n");
    prints(" Current Time: ");
    printc('0' + (current_time.hours / 10));
    printc('0' + (current_time.hours % 10));
    printc(':');
    printc('0' + (current_time.minutes / 10));
    printc('0' + (current_time.minutes % 10));
    printc(':');
    printc('0' + (current_time.seconds / 10));
    printc('0' + (current_time.seconds % 10));
    prints("\n");
}

static void cmd_uptime(void)
{
    rtc_time_t current_time = rtc_get_time();
    rtc_time_t boot_time = rtc_boottime();
    uint32_t uptime_ms = rtc_calculate_uptime(&boot_time, &current_time);
    uint32_t uptime_seconds = uptime_ms / 1000;

    uint32_t hours = uptime_seconds / 3600;
    uint32_t minutes = (uptime_seconds % 3600) / 60;
    uint32_t seconds = uptime_seconds % 60;

    prints(" System uptime: ");
    printc('0' + (hours / 10));
    printc('0' + (hours % 10));
    prints("h ");
    printc('0' + (minutes / 10));
    printc('0' + (minutes % 10));
    prints("m ");
    printc('0' + (seconds / 10));
    printc('0' + (seconds % 10));
    prints("s\n");
}

static void cmd_echo(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        prints(argv[i]);
        if (i < argc - 1)
        {
            printc(' ');
        }
    }
    printc('\n');
}

static void cmd_ls(void)
{
    f_list();
}

static void cmd_pwd(void)
{
    char cwd[256];
    f_get_cwd(cwd, sizeof(cwd));
    prints(" ");
    prints(cwd);
    prints("\n");
}

static void cmd_cd(int argc, char *argv[])
{
    if (argc < 2)
    {
        prints(" Usage: cd <directory>\n");
        return;
    }

    if (f_chdir(argv[1]) == SFS_OK)
    {
        prints(" Changed directory to: ");
        prints(argv[1]);
        prints("\n");
    }
    else
    {
        prints(" Failed to change directory: ");
        prints(argv[1]);
        prints("\n");
    }
}

static void cmd_mkdir(int argc, char *argv[])
{
    if (argc < 2)
    {
        prints(" Usage: mkdir <directory>\n");
        return;
    }

    if (f_mkdir(argv[1]) == SFS_OK)
    {
        prints(" Directory created: ");
        prints(argv[1]);
        prints("\n");
    }
    else
    {
        prints(" Failed to create directory: ");
        prints(argv[1]);
        prints("\n");
    }
}

static void cmd_rmdir(int argc, char *argv[])
{
    if (argc < 2)
    {
        prints(" Usage: rmdir <directory>\n");
        return;
    }

    if (f_rmdir(argv[1]) == SFS_OK)
    {
        prints(" Directory removed: ");
        prints(argv[1]);
        prints("\n");
    }
    else
    {
        prints(" Failed to remove directory: ");
        prints(argv[1]);
        prints("\n");
    }
}

static void cmd_touch(int argc, char *argv[])
{
    if (argc < 2)
    {
        prints(" Usage: touch <filename>\n");
        return;
    }

    if (f_create(true, argv[1], 1024) == SFS_OK)
    {
        sfs_file_t file;
        if (f_open(true, argv[1], &file) != SFS_OK)
        {
            prints(" Failed to open file for zeroing: ");
            prints(argv[1]);
            prints("\n");
            return;
        }

        char zeros[1024];
        for (int i = 0; i < 1024; i++)
            zeros[i] = '\0';

        if (f_write(true, &file, zeros, 1024) != SFS_OK)
        {
            prints(" Failed to allocate file: ");
            prints(argv[1]);
            prints("\n");
            f_close(true, &file);
            return;
        }

        f_close(true, &file);
        prints(" File created: ");
        prints(argv[1]);
        prints("\n");
    }
    else
    {
        prints(" Failed to create file: ");
        prints(argv[1]);
        prints("\n");
    }
}

static void cmd_rm(int argc, char *argv[])
{
    if (argc < 2)
    {
        prints(" Usage: rm <filename>\n");
        return;
    }

    if (f_rm(true, argv[1]) == SFS_OK)
    {
        prints(" File deleted: ");
        prints(argv[1]);
        prints("\n");
    }
    else
    {
        prints(" Failed to delete file: ");
        prints(argv[1]);
        prints("\n");
    }
}

static void cmd_cat(int argc, char *argv[])
{
    if (argc < 2)
    {
        prints(" Usage: cat <filename>\n");
        return;
    }

    sfs_file_t file;
    if (f_open(true, argv[1], &file) != SFS_OK)
    {
        prints(" Failed to open file: ");
        prints(argv[1]);
        prints("\n");
        return;
    }

    char buffer[1025];
    uint32_t bytes_read;

    if (f_read(true, &file, buffer, 1024, &bytes_read) == SFS_OK)
    {
        buffer[bytes_read] = '\0';
        prints(" ");
        prints(buffer);
        prints("\n");
    }
    else
    {
        prints(" Failed to read file: ");
        prints(argv[1]);
        prints("\n");
    }

    f_close(true, &file);
}

static void cmd_exec(int argc, char *argv[])
{
    if (argc < 2)
    {
        prints(" Usage: exec <filename> [args...]\n");
        return;
    }

    prints(" Loading ELF binary: ");
    prints(argv[1]);
    prints("\n");

    int ret = exec(argv[1], argc - 1, &argv[1]);

    if (ret < 0)
    {
        prints(" Failed to execute ELF (error code: ");
        if (ret < -9)
        {
            printc('-');
            printc('0' + ((-ret) / 10));
            printc('0' + ((-ret) % 10));
        }
        else if (ret < 0)
        {
            printc('-');
            printc('0' + (-ret));
        }
        prints(")\n");
    }
}

static void cmd_write(int argc, char *argv[])
{
    if (argc < 3)
    {
        prints(" Usage: write <filename> <content>\n");
        return;
    }

    char content[1024];
    content[0] = '\0';

    for (int i = 2; i < argc; i++)
    {
        if (i > 2)
            strncat(content, " ", sizeof(content) - strlen(content) - 1);
        strncat(content, argv[i], sizeof(content) - strlen(content) - 1);
    }

    sfs_file_t file;
    if (f_open(true, argv[1], &file) != SFS_OK)
    {
        prints(" Failed to open file: ");
        prints(argv[1]);
        prints("\n");
        return;
    }

    if (f_write(true, &file, content, strlen(content)) == SFS_OK)
    {
        prints(" File written: ");
        prints(argv[1]);
        prints("\n");
    }
    else
    {
        prints(" Failed to write file: ");
        prints(argv[1]);
        prints("\n");
    }

    f_close(true, &file);
}

static void help(void)
{
    prints("\n Help:\n\n");
    prints(" - help       - Display this menu\n");
    prints(" - clear      - Clear the screen\n");
    prints(" - echo       - Repeat following text\n");
    prints(" - version    - Display embedded version\n");
    prints(" - date       - Show current date\n");
    prints(" - uptime     - Show system uptime\n");
    prints(" - shutdown   - Shutdown the system\n");
    prints(" - exit       - Shutdown the system\n");
    prints("\n File System Commands:\n");
    prints(" - ls         - List files and directories\n");
    prints(" - pwd        - Show current directory\n");
    prints(" - cd         - Change directory\n");
    prints(" - mkdir      - Create a new directory\n");
    prints(" - rmdir      - Remove an empty directory\n");
    prints(" - touch      - Create a new file (1024 bytes)\n");
    prints(" - rm         - Delete a file\n");
    prints(" - cat        - Show file contents\n");
    prints(" - write      - Write data to a file as plain text\n");
    prints(" - format     - Format the disk\n");
    prints(" - fsstat     - Show StartFS statistics\n");
    prints("\n System Commands:\n");
    prints(" - exec       - Execute an ELF file\n");
}

bool shell_execute(const char *command)
{
    char cmd_copy[256];
    strcpy(cmd_copy, command);
    char *argv[32];
    int argc = parse_command(cmd_copy, argv);

    if (argc == 0)
    {
        return true;
    }
    else if (strcmp(argv[0], "help") == 0)
    {
        help();
    }
    else if (strcmp(argv[0], "clear") == 0)
    {
        clr();
    }
    else if (strcmp(argv[0], "echo") == 0)
    {
        cmd_echo(argc, argv);
    }
    else if (strcmp(argv[0], "version") == 0)
    {
        cmd_version();
    }
    else if (strcmp(argv[0], "date") == 0)
    {
        cmd_date();
    }
    else if (strcmp(argv[0], "uptime") == 0)
    {
        cmd_uptime();
    }
    else if (strcmp(argv[0], "ls") == 0)
    {
        cmd_ls();
    }
    else if (strcmp(argv[0], "pwd") == 0)
    {
        cmd_pwd();
    }
    else if (strcmp(argv[0], "cd") == 0)
    {
        cmd_cd(argc, argv);
    }
    else if (strcmp(argv[0], "mkdir") == 0)
    {
        cmd_mkdir(argc, argv);
    }
    else if (strcmp(argv[0], "rmdir") == 0)
    {
        cmd_rmdir(argc, argv);
    }
    else if (strcmp(argv[0], "format") == 0)
    {
        f_format(0);
    }
    else if (strcmp(argv[0], "touch") == 0)
    {
        cmd_touch(argc, argv);
    }
    else if (strcmp(argv[0], "rm") == 0)
    {
        cmd_rm(argc, argv);
    }
    else if (strcmp(argv[0], "fsstat") == 0)
    {
        f_print_stats();
    }
    else if (strcmp(argv[0], "exit") == 0 || strcmp(argv[0], "shutdown") == 0)
    {
        return false;
    }
    else if (strcmp(argv[0], "cat") == 0)
    {
        cmd_cat(argc, argv);
    }
    else if (strcmp(argv[0], "write") == 0)
    {
        cmd_write(argc, argv);
    }
    else if (strcmp(argv[0], "exec") == 0)
    {
        cmd_exec(argc, argv);
    }
    else
    {
        prints(" Unknown command: ");
        prints(argv[0]);
        prints("\n");
    }
    log("Command executed: %s.", 1, 0, command);
    return true;
}

int main(int argc, char **argv, kernel_api_t *api)
{
    g_api = api;
    asm volatile("cli");
    while (1)
    {
        setcolor(0x00FF00, 0x000000);
        prints("\nStartOS:~$ ");
        setcolor(0xffffff, 0x000000);
        asm volatile("sti");
        read_line(command_buffer, 256, true);
        asm volatile("cli");
        printc('\n');
        if (shell_execute(command_buffer) == false)
            return 0;
        memset(command_buffer, 0, 256);
    }
    return 0;
}