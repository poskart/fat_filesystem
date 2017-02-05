/*
 * fat_filesystem.h
 *
 *  Created on: 14 sty 2017
 *      Author: piotr
 */

#ifndef FAT_FILESYSTEM_H_
#define FAT_FILESYSTEM_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define BLOCK_SIZE 2048
#define MAX_NAME_LENGTH 24
#define RESET_BLOCK_SIZE 256

typedef int fat_table_type;

struct fat_file_system
{
	FILE * pFile;
	unsigned int blocks_count;
	fat_table_type * fat_table;
	struct fat_inode * inodes;
};

struct fat_superblock
{
	size_t size;
	size_t free_blocks_count;
	size_t total_block_count;
};

struct fat_inode
{
	char name[MAX_NAME_LENGTH];
	unsigned int file_size;
	fat_table_type first_file_block;
};

struct fat_file_system * create_fat_fs(const char * file_name, const size_t filesize);
struct fat_file_system * mount_fat_fs(const char * file_name);
void unmount_fat_fs(struct fat_file_system * fs);
void delete_fat_fs(const char * file_name);
void dump_fat_fs(const struct fat_file_system * fs);
void list_files_fat_fs(const struct fat_file_system * fs);
int copy_to_fat_fs(const struct fat_file_system * fs, const char * source_file_name, const char * dest_file_name);
int copy_from_fat_fs(const struct fat_file_system * fs, const char * source_file_name, const char * dest_file_name);
void delete_file_fat_fs(const struct fat_file_system * fs, const char * file_name);
size_t get_free_blocks_count(const struct fat_file_system * fs);
size_t block_count_from_space(const size_t total_space_size);
size_t get_block_position_in_bytes(const struct fat_file_system * fs, size_t block_number);
int compute_required_blocks_count(const int file_size, const int block_size);

#endif /* FAT_FILESYSTEM_H_ */
