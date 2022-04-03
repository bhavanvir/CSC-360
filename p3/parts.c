#include <stdio.h>
#include <string.h>
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

void diskinfo(int argc, char *argv[])
{
    // Check if the user input is valid
    if (argc != 2)
    {
        printf("Expected: ./diskinfo <disk image>\n");
        exit(1);
    }

    // Open the disk image for reading and writing
    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
        perror("Error at fd");

    // Return information about the disk image
    struct stat buffer;
    fstat(fd, &buffer);

    // Create a new mapping in the virtual address space
    void *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address == (void *)-1)
        perror("Error at address");

    // Initialize a new super block structure with the mapping of the address space
    struct superblock_t *sb;
    sb = (struct superblock_t *)address;

    // Initialize the start and end location variables to their corresponding values in the FAT table
    int start = ntohl(sb->fat_start_block) * htons(sb->block_size);
    int end = ntohl(sb->fat_block_count) * htons(sb->block_size);
    // Initialize the number of free, reserved and allocated blocks to 0 initially
    int free, reserved, allocated = 0;

    // Loop from the start of the FAT black to the end, incrementing by 4 each time
    for (int i = start; i < start + end; i += 4)
    {
        int val = 0;
        memcpy(&val, address + i, 4);
        val = htonl(val);
        // Depending on the value at the address + i space, increment the appropriate variable
        val == 0 ? free++ : val == 1 ? reserved++
                                     : allocated++;
    }

    // Print the appropriate super block information
    printf("Super block information:\n");
    printf("Block size: %d\n", htons(sb->block_size));
    printf("Block count: %d\n", ntohl(sb->file_system_block_count));
    printf("FAT starts: %d\n", ntohl(sb->fat_start_block));
    printf("FAT blocks: %d\n", ntohl(sb->fat_block_count));
    printf("Root directory start: %d\n", ntohl(sb->root_dir_start_block));
    printf("Root directory blocks: %d\n\n", ntohl(sb->root_dir_block_count));

    // Print the appropriate FAT information
    printf("FAT information:\n");
    printf("Free Blocks: %d\n", free);
    printf("Reserved Blocks: %d\n", reserved);
    printf("Allocated Blocks: %d\n", allocated);

    // Delete the mappings for the address range and close the file
    munmap(address, buffer.st_size);
    close(fd);
}

void disklist(int argc, char **argv)
{
    if (argc != 3 || strcmp(argv[2], "/") != false)
    {
        printf("Expected: ./disklist <disk image> /\n");
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

    int size = htons(sb->block_size);
    int start = ntohl(sb->root_dir_start_block) * size;

    // Create a new variable corresponding to the rootblock of the directory entry
    struct dir_entry_t *rb;

    // Loop from the start of the block to the end, incrementing by 64 each time
    for (int i = start; i < start + size; i += 64)
    {
        // Set the rootblock variable to the value of the memory location at address + i
        rb = (struct dir_entry_t *)(address + i);
        // If the size of the rootblock is 0, skip the entry entirely
        if (ntohl(rb->size) == 0)
            continue;
        // Print the correct information pertaining to the disk image
        printf("%c %10d %30s %4d/%02d/%02d %02d:%02d:%02d\n", rb->status == 3 ? 'F' : 'D', ntohl(rb->size), rb->filename, htons(rb->modify_time.year),
               rb->modify_time.month, rb->modify_time.day, rb->modify_time.hour, rb->modify_time.minute, rb->modify_time.second);
    }

    munmap(address, buffer.st_size);
    close(fd);
}

void diskget(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Expected: ./diskget <disk image> /<disk file> <local file>\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
        perror("Error at fd");

    struct stat buffer;
    fstat(fd, &buffer);

    // Grab the name of the file and strip the "/" character from the input
    char *file_name = argv[2];
    char *last_arg = argv[3];
    // If there is no "/" character in the appropriate position, exit in error and notify the user
    if (file_name[0] != 47 || last_arg[0] == 47)
    {
        printf("Expected: ./diskget <disk image> /<disk file> <local file>\n");
        exit(1);
    }
    memmove(&file_name[0], &file_name[1], strlen(file_name));

    // Initialize a buffer with the maximum amount of readable information
    char file_data[1000];
    strcpy(file_data, file_name);

    void *address = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address == (void *)-1)
        perror("Error at address");

    struct superblock_t *sb;
    sb = (struct superblock_t *)address;

    int size = htons(sb->block_size);
    int start = htonl(sb->root_dir_start_block) * size;
    void *file = mmap(NULL, htonl(sb->file_system_block_count) * size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    // Initialize variables for the file size, status and file_found boolean
    int fs;
    char status;
    bool file_found = false;

    int idx = 0;
    // Loop from the initial 0th index to the start of the block, incrementing by 64 each time
    for (int i = 0; i < start; i += 64)
    {
        // Copy the status of the address space into the status varialbe
        memcpy(&status, file + start + i, 1);

        // Create a temporary buffer to copy information from certain address spaces
        char tmp[1000];
        memcpy(&fs, file + start + i + 9, 4);
        fs = ntohl(fs);
        for (int j = 27; j < 54; j++)
        {
            // Copy the status of the current address space into the status variable each loop
            memcpy(&status, file + start + i + j, 1);
            // Set the current index value to the status variable
            tmp[idx] = status;
            // Increment the current working index
            idx++;
        }
        idx = 0;

        // If the file_data and temporary variable contain the same information then continue
        if (strcmp(tmp, file_data) == false)
        {
            // The file has been found
            file_found = true;

            // Open the appopriate file on the user's local machine in writing mode
            FILE *out = fopen(argv[3], "w");

            // Copy data from the appropriate location in memory and store it within the file size variable
            memcpy(&fs, file + start + i + 5, 4);
            // Convert the unsigned integer to nhost byte order, after each memory copy operation
            fs = ntohl(fs);
            memcpy(&fs, file + start + i + 9, 4);
            fs = ntohl(fs);
            int file_size = fs;
            memcpy(&fs, file + start + i + 1, 4);
            fs = ntohl(fs);

            char file_buffer;
            // While the file size variable doesn't result in error, particularly reaching the end of the file, continue looping
            while (fs != -1)
            {
                // If the current file size is greater than the total file size continue writing to the appropriate output file
                if (file_size > size)
                {
                    for (int i = 0; i < size; i++)
                    {
                        memcpy(&file_buffer, file + size * fs + i, 1);
                        fwrite(&file_buffer, sizeof(char), 1, out);
                    }
                    // Decrement the current file size by the total file size, to get the remaining total
                    file_size -= size;
                }
                // If not, continue writing to the appropriate output file
                else
                {
                    for (int i = 0; i < file_size; i++)
                    {
                        memcpy(&file_buffer, file + size * fs + i, 1);
                        fwrite(&file_buffer, sizeof(char), 1, out);
                    }
                }

                // Copy data from the final address space in the block to the file size vriable
                memcpy(&fs, file + htonl(sb->fat_start_block) * size + fs * 4, 4);
                fs = ntohl(fs);
            }

            fclose(out);
        }
    }
    // If the file is not found return an error; else notify the user of successful completion
    if (file_found == false)
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
    if (argc != 4)
    {
        printf("Expected: ./diskput <disk image> <local file> /<disk file>\n");
        exit(1);
    }

    int fd = open(argv[1], O_RDWR);
    if (fd == -1)
        perror("Error at fd");

    struct stat buffer_tp;
    fstat(fd, &buffer_tp);

    int f_tu = open(argv[2], O_RDWR);
    if (f_tu == -1)
    {
        printf("Error: file not found\n");
        exit(1);
    }

    struct stat buffer_tu;
    fstat(f_tu, &buffer_tu);

    char *file_name = argv[3];
    if (file_name[0] != 47)
    {
        printf("Expected: ./diskput <disk image> <local file> /<disk file>\n");
        exit(1);
    }
    memmove(&file_name[0], &file_name[1], strlen(file_name));
    int file_size = buffer_tu.st_size;

    FILE *fp;
    fp = fopen(argv[2], "r");

    void *address = mmap(NULL, buffer_tp.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (address == (void *)-1)
        perror("Error at address");

    struct superblock_t *sb;
    sb = (struct superblock_t *)address;

    int size = htons(sb->block_size);
    int start = ntohl(sb->fat_start_block) * size;
    int block = ntohl(sb->root_dir_start_block) * size;
    int end = (ntohl(sb->fat_start_block) + ntohl(sb->fat_block_count)) * size;

    // Create a variable declaring the needed file size by computing a modulo operation on the remnant value, and incrementing appropriately
    int need = file_size / size;
    file_size % size != 0 ? need++ : need;

    // Declare a variable with the amount of memory used
    int used = 0;
    int memory_idx = 0, starting_idx = 0;
    char tmp[1000];
    // Loop from the beginning of the block to the end, incrementing by 4 each time
    for (int i = start; i < end; i += 4)
    {
        int idx = 0;
        // Copy the value of the address space into the current index value
        memcpy(&idx, address + i, 4);
        // Convert the unsigned integer to network byte order
        idx = htonl(idx);
        if (idx == 0)
        {
            // If no memory has been currently used, update the starting index and read from the appropriate local file
            if (used == 0)
            {
                starting_idx = (i - start) / 4;
                used++;

                fread(tmp, size, 1, fp);
                memcpy(address + (starting_idx * size), &tmp, size);
            }
            // Update the memory index location
            memory_idx = (i - start) / 4;

            // Read from the local file and copy the apportiate data from the location in memory
            fread(tmp, size, 1, fp);
            memcpy(address + (memory_idx * size), &tmp, size);

            // Convert the memory index variable from an unsigned integer to network byte order
            memory_idx = htonl(memory_idx);
            memcpy(address + i, &memory_idx, 4);

            // Increment the amount of memory used
            used++;
        }
    }

    // Set the status of the file added to the disk image as "F"
    int status = 3;
    // Loop from the beginning of the file to the end, incrementing by 64 each time
    for (int i = block; i < block + size; i += 64)
    {
        int idx = 0;
        // Copy the data from the current address space into the index value
        memcpy(&idx, address + i, 1);

        if (idx == 0)
        {
            // Get the value of the local time zone
            time_t rawtime;
            time(&rawtime);
            // From time.h create a structure used to hold the time and date
            struct tm *info = localtime(&rawtime);

            // Create a buffer that holds the time information
            char buffer_time[10];
            int time;

            // Set the added file in the disk image as "F" for the type
            memcpy(address + i, &status, 1);

            // Set the attributes for the file added in the disk image
            starting_idx = ntohl(starting_idx);
            memcpy(address + i + 1, &starting_idx, 4);

            file_size = htonl(file_size);
            memcpy(address + i + 9, &file_size, 4);

            // Get the current year
            strftime(buffer_time, sizeof(buffer_time), "%Y", info);
            sscanf(buffer_time, "%d", &time);
            // Convert the unsigned short interger to a network byte
            time = htons(time);
            memcpy(address + i + 20, &time, 2);

            // Get the current month
            strftime(buffer_time, sizeof(buffer_time), "%m", info);
            sscanf(buffer_time, "%d", &time);
            memcpy(address + i + 22, &time, 1);

            // Get the current day
            strftime(buffer_time, sizeof(buffer_time), "%d", info);
            sscanf(buffer_time, "%d", &time);
            memcpy(address + i + 23, &time, 1);

            // Get the current hour
            strftime(buffer_time, sizeof(buffer_time), "%H", info);
            sscanf(buffer_time, "%d", &time);
            memcpy(address + i + 24, &time, 1);

            // Get the current month
            strftime(buffer_time, sizeof(buffer_time), "%M", info);
            sscanf(buffer_time, "%d", &time);
            memcpy(address + i + 25, &time, 1);

            // Get the current seconds
            strftime(buffer_time, sizeof(buffer_time), "%S", info);
            sscanf(buffer_time, "%d", &time);
            memcpy(address + i + 26, &time, 1);

            // Set the desired file name in the disk image
            strncpy(address + i + 27, file_name, 31);

            break;
        }
    }
    // Notify the user of what the file was placed as in the disk image
    printf("Success: placed %s as %s in %s\n", argv[2], argv[3], argv[1]);

    munmap(address, buffer_tp.st_size);
    close(fd);
    close(f_tu);
    fclose(fp);
}

void diskfix(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Expected: ./diskfix <disk image>\n");
        exit(1);
    }
    printf("Not attempted 😭\n");
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