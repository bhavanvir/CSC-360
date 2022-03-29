#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

// Super block: adapted from the tutorial slides
struct __attribute__((__packed__)) superblock_t
{
    uint8_t fs_id[8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};

// Time and date entry: adapted from the tutorial slides
struct __attribute__((__packed__)) dir_entry_timedate_t
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// Directory entry: adapted from the tutorial slides
struct __attribute__((__packed__)) dir_entry_t
{
    uint8_t status;
    uint32_t starting_block;
    uint32_t block_count;
    uint32_t size;
    struct dir_entry_timedate_t create_time;
    struct dir_entry_timedate_t modify_time;
    uint8_t filename[31];
    uint8_t unused[6];
};

char **tokenize_dir(char *dir, int *num_dir)
{
    size_t init_size = 2;

    char *t = strtok(dir, "/");
    char **t_dir = malloc(sizeof(char *));

    int i = 0;
    while (t != NULL)
    {
        if (i >= init_size)
        {
            init_size *= 2;
            char **tmp = (char **)realloc(t_dir, init_size * sizeof(char *));

            if (tmp == NULL)
                perror("Error at tmp");

            t_dir = tmp;
        }
        t_dir[i++] = t;
        t = strtok(NULL, "/");
    }
    *num_dir = i;
    t_dir[i] = NULL;

    return t_dir;
}

void diskinfo(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Error: expected a disk image in the form *.img");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
        perror("Error at fd");

    struct stat buffer;
    fstat(fd, &buffer);

    void *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address == (void *)-1)
        perror("Error at address");

    struct superblock_t *sb;
    sb = (struct superblock_t *)address;

    int start = ntohl(sb->fat_start_block) * htons(sb->block_size);
    int end = ntohl(sb->fat_block_count) * htons(sb->block_size);
    int free, reserved, allocated = 0;

    for (int i = start; i < start + end; i += 4)
    {
        int val = 0;
        memcpy(&val, address + i, 4);
        val = htonl(val);
        val == 0 ? free++ : val == 1 ? reserved++
                                     : allocated++;
    }

    printf("Super block information:\n");
    printf("Block size: %d\n", htons(sb->block_size));
    printf("Block count: %d\n", ntohl(sb->file_system_block_count));
    printf("FAT starts: %d\n", ntohl(sb->fat_start_block));
    printf("FAT blocks: %d\n", ntohl(sb->fat_block_count));
    printf("Root directory start: %d\n", ntohl(sb->root_dir_start_block));
    printf("Root directory blocks: %d\n\n", ntohl(sb->root_dir_block_count));

    printf("FAT information:\n");
    printf("Free Blocks: %d\n", free);
    printf("Reserved Blocks: %d\n", reserved);
    printf("Allocated Blocks: %d\n", allocated);

    munmap(address, buffer.st_size);
    close(fd);
}

void disklist(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Error: expected a disk image directory\n");
        exit(1);
    }

    char **tokens = {'\0'};
    int num_dir = 0;
    if (argc == 3)
        tokens = tokenize_dir(argv[2], &num_dir);

    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
        perror("Error at fd");

    struct stat buffer;
    fstat(fd, &buffer);

    void *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address == (void *)-1)
        perror("Error at address");

    struct superblock_t *sb;
    sb = (struct superblock_t *)address;

    int size = htons(sb->block_size);
    int start = ntohl(sb->root_dir_start_block) * size;

    struct dir_entry_t *rb;

    int idx = 0;

    for (;;)
    {
        for (int i = start; i < start + size; i += 64)
        {
            if (argc == 2)
            {
                for (int j = start; j < start + size; j += 64)
                {
                    rb = (struct dir_entry_t *)(address + j);
                    if (ntohl(rb->size) == 0)
                        continue;
                    printf("%c %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n", rb->status == 3 ? 'F' : 'D', ntohl(rb->size), rb->filename, htons(rb->modify_time.year),
                           rb->modify_time.month, rb->modify_time.day, rb->modify_time.hour, rb->modify_time.minute, rb->modify_time.second);
                }
                return;
            }
            else
            {
                rb = (struct dir_entry_t *)(address + i);
                if (strcmp((const char *)rb->filename, tokens[idx]) == false)
                {
                    idx++;
                    start = ntohl(rb->starting_block) * size;
                    if (idx == num_dir)
                    {
                        for (int j = start; j < start + size; j += 64)
                        {
                            rb = (struct dir_entry_t *)(address + j);
                            if (ntohl(rb->size) == 0)
                                continue;
                            printf("%c %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n", rb->status == 3 ? 'F' : 'D', ntohl(rb->size), rb->filename, htons(rb->modify_time.year),
                                   rb->modify_time.month, rb->modify_time.day, rb->modify_time.hour, rb->modify_time.minute, rb->modify_time.second);
                        }
                        return;
                    }
                }
            }
        }
    }

    munmap(address, buffer.st_size);
    close(fd);
}

void diskget(int argc, char *argv[])
{
    if (argv[2] == NULL || argv[3] == NULL)
    {
        printf("Error: too few arguments\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
        perror("Error at fd");

    struct stat buffer;
    fstat(fd, &buffer);

    char file_data[1000];
    strcpy(file_data, argv[2]);

    void *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address == (void *)-1)
        perror("Error at address");

    struct superblock_t *sb;
    sb = (struct superblock_t *)address;

    int size = htons(sb->block_size);
    int start = htonl(sb->root_dir_start_block) * size;
    void *file = mmap(NULL, htonl(sb->file_system_block_count) * size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    int fssize;
    char stat;

    bool flag = true;

    int idx = 0;
    for (int i = 0; i < start; i += 64)
    {
        memcpy(&stat, file + start + i, 1);

        char tmp[1000];
        memcpy(&fssize, file + start + i + 9, 4);
        fssize = ntohl(fssize);
        for (int name_count = 27; name_count < 54; name_count++)
        {
            memcpy(&stat, file + start + i + name_count, 1);
            tmp[idx] = stat;
            idx++;
        }
        idx = 0;

        if (strcmp(tmp, file_data) == 0)
        {
            flag = false;

            FILE *out = fopen(argv[3], "w");

            memcpy(&fssize, file + start + i + 5, 4);
            fssize = ntohl(fssize);

            memcpy(&fssize, file + start + i + 9, 4);
            fssize = ntohl(fssize);
            int filesize = fssize;

            memcpy(&fssize, file + start + i + 1, 4);
            fssize = ntohl(fssize);

            char file_buffer;
            while (fssize != -1)
            {

                if (filesize > size)
                {
                    filesize -= size;
                    for (int i = 0; i < size; i++)
                    {
                        memcpy(&file_buffer, file + size * fssize + i, 1);
                        fwrite(&file_buffer, sizeof(char), 1, out);
                    }
                }
                else
                {
                    for (int i = 0; i < filesize; i++)
                    {
                        memcpy(&file_buffer, file + size * fssize + i, 1);
                        fwrite(&file_buffer, sizeof(char), 1, out);
                    }
                }

                memcpy(&fssize, file + htonl(sb->fat_start_block) * size + fssize * 4, 4);
                fssize = ntohl(fssize);
            }
        }
    }
    if (flag == true)
    {
        printf("Error: file not found\n");
        exit(1);
    }
    else
        printf("Success: found %s in %s\n", argv[2], argv[1]);

    munmap(address, buffer.st_size);
    close(fd);
}

void diskput(int argc, char *argv[])
{
}

void diskfix(int argc, char *argv[])
{
    printf("Not completed\n");
}

int main(int argc, char *argv[])
{
#if defined(PART1)
    diskinfo(argc, argv);
#elif defined(PART2)
    disklist(argc, argv);
#elif defined(PART3)
    diskget(argc, argv);
#elif defined(PART4)
    diskput(argc, argv);
#elif defined(PART5)
    diskfix(argc, argv);
#else
#error "PART[12345] must be defined"
#endif
    return 0;
}