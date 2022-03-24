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

void print_output(struct superblock_t *sb, int free, int reserved, int allocated, char *flag)
{
    if (strcmp(flag, "BLOCK") == 0)
    {
        printf("Super block information:\n");
        printf("Block size: %d\n", htons(sb->block_size));
        printf("Block count: %d\n", ntohl(sb->file_system_block_count));
        printf("FAT starts: %d\n", ntohl(sb->fat_start_block));
        printf("FAT blocks: %d\n", ntohl(sb->fat_block_count));
        printf("Root directory start: %d\n", ntohl(sb->root_dir_start_block));
        printf("Root directory blocks: %d\n\n", ntohl(sb->root_dir_block_count));
    }
    else if (strcmp(flag, "FAT") == 0)
    {
        printf("FAT information:\n");
        printf("Free Blocks: %d\n", free);
        printf("Reserved Blocks: %d\n", reserved);
        printf("Allocated Blocks: %d\n", allocated);
    }
}

void diskinfo(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Error: expected a disk image in the form *.img");
        exit(1);
    }

    int fp = open(argv[1], O_RDWR);
    if (fp == -1)
        perror("Error at fp");

    struct stat buf;
    fstat(fp, &buf);

    void *p = mmap(NULL, buf.st_size, PROT_WRITE | PROT_READ, MAP_SHARED, fp, 0);
    if (p == (void *)-1)
        perror("Error at p");

    struct superblock_t *sb;
    sb = (struct superblock_t *)p;

    int start = ntohl(sb->fat_start_block) * htons(sb->block_size);
    int end = ntohl(sb->fat_block_count) * htons(sb->block_size);
    int free, reserved, allocated = 0;

    for (int i = start; i < start + end; i += 4)
    {
        int val = 0;
        memcpy(&val, p + i, 4);
        val = htonl(val);
        val == 0 ? free++ : val == 1 ? reserved++
                                     : allocated++;
    }

    print_output(sb, free, reserved, allocated, "BLOCK");
    print_output(sb, free, reserved, allocated, "FAT");

    munmap(p, buf.st_size);
    close(fp);
}

void disklist(int argc, char *argv[])
{
}

void diskget(int argc, char *argv[])
{
}

void diskput(int argc, char *argv[])
{
}

void diskfix(int argc, char *argv[])
{
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