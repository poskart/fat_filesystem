/*
 * fat_filesystem.c
 *
 *  Created on: 14 sty 2017
 *      Author: piotr
 */

#include "fat_filesystem.h"

/*
 * Create fat file system with the given size
 */
struct fat_file_system * create_fat_fs(const char * file_name, const size_t filesize)
{
	FILE * pFile;
	char reset_block[RESET_BLOCK_SIZE];
	size_t remaining_bytes, bytes_to_save, blocks_count, i;
	struct fat_file_system * fat_fs;
	struct fat_superblock spblock;

	if(!(pFile = fopen(file_name, "wb")))
			return NULL;

	// initialize reset block with zeros
	memset(reset_block, 0, sizeof(reset_block));

	// reset entire file (file system) space to zero
	remaining_bytes = filesize;
	while(remaining_bytes > 0)
	{
		bytes_to_save = remaining_bytes < RESET_BLOCK_SIZE? remaining_bytes : RESET_BLOCK_SIZE;
		fwrite(reset_block, 1, bytes_to_save, pFile);
		remaining_bytes -= bytes_to_save;
	}

	// assign space to blocks and get blocks count
	blocks_count = block_count_from_space(filesize);

	spblock.total_block_count = blocks_count;
	spblock.free_blocks_count = blocks_count;
	spblock.size = filesize;
	fseek(pFile, 0, SEEK_SET);
	fwrite(&spblock, sizeof(spblock), 1, pFile);


	fat_fs = (struct fat_file_system *)malloc(sizeof(struct fat_file_system));
	fat_fs->pFile = pFile;
	fat_fs->blocks_count = blocks_count;
	fat_fs->fat_table = (fat_table_type *)malloc(blocks_count * sizeof(fat_table_type));
	fat_fs->inodes = (struct fat_inode *)malloc(blocks_count * sizeof(struct fat_inode));

	// reset i-nodes and FAT table
	for(i = 0 ; i < blocks_count; i++)
	{
		fat_fs->fat_table[i] = -2;
		fat_fs->inodes[i].first_file_block = -1;
	}
	return fat_fs;
}

/*
 * Mount fat file system
 */
struct fat_file_system * mount_fat_fs(const char * file_name)
{
	FILE * pFile;
	size_t file_size;
	struct fat_file_system * fs;
	struct fat_superblock spblock;

	if(!(pFile = fopen(file_name, "r+b")))
		return NULL;
	fseek(pFile, 0, SEEK_END);
	file_size = ftell(pFile);
	if(file_size < sizeof(struct fat_superblock))
	{
		fclose(pFile);
		return NULL;
	}
	// allocate space for file_system structure
	fs = malloc(sizeof(struct fat_file_system));
	// go to file begin
	fseek(pFile, 0, SEEK_SET);
	if(fread(&spblock, sizeof(spblock), 1, pFile) <= 0)
	{
		fclose(pFile);
		return NULL;
	}
	fs->blocks_count = spblock.total_block_count;

	// allocate memory for i-nodes table and read i-nodes from the file
	fs->inodes = malloc(sizeof(struct fat_inode) * fs->blocks_count);
	if(!fread(fs->inodes, sizeof(struct fat_inode), fs->blocks_count, pFile))
	{
		fclose(pFile);
		return NULL;
	}

	// allocate memory for fat table and read fat table from the file
	fs->fat_table = malloc(sizeof(fat_table_type) * fs->blocks_count);
	if(!fread(fs->fat_table, sizeof(fat_table_type), fs->blocks_count, pFile))
	{
		fclose(pFile);
		return NULL;
	}
	fs->pFile = pFile;

	return fs;
}

/*
 * Unmount fat file system
 */
void unmount_fat_fs(struct fat_file_system * fs)
{
	int a, b;
	// reposition stream position indicator to first byte behind superblock
	fseek(fs->pFile, sizeof(struct fat_superblock), SEEK_SET);
	// write all inodes to the file system
	a = fwrite(fs->inodes, sizeof(struct fat_inode), fs->blocks_count, fs->pFile);
	// write fat allocation table to the file
	b = fwrite(fs->fat_table, sizeof(fat_table_type), fs->blocks_count, fs->pFile);

	fclose(fs->pFile);
	free(fs->fat_table);
	free(fs->inodes);
	free(fs);
	fs = NULL;
}

/*
 * Delete file system
 */
void delete_fat_fs(const char * file_name)
{
	unlink(file_name);
}

/*
 * Show memory map
 */
void dump_fat_fs(const struct fat_file_system * fs)
{
	size_t occupied_blocks_count;
	int fat_index;
	printf("I - nodes list:\n");
	unsigned int i;
	for(i = 0; i < fs->blocks_count; i++)
	{
		printf("Id: %d\n", i);
		printf("Name: %s\n", fs->inodes[i].name);
		printf("File size: %d\n", fs->inodes[i].file_size);
		printf("First data block: %d\n", fs->inodes[i].first_file_block);
		printf("\n");
	}
	printf("Files occupy blocks:\n");
	occupied_blocks_count = 0;
	for(i = 0; i < fs->blocks_count; i++)
	{
		if(fs->fat_table[i] > -2)
			occupied_blocks_count++;
		fat_index = fs->inodes[i].first_file_block;
		if(fat_index >= 0)
		{
			printf("%s\t", fs->inodes[i].name);
			while(fat_index >= 0)
			{
				printf("%d, ", (int)fat_index);
				fat_index = fs->fat_table[fat_index];
			}
			printf("\n");
		}
	}
	printf("\nFAT table:\n");
	for(i = 0; i < fs->blocks_count; i++)
	{
		printf("%d, ", fs->fat_table[i]);
	}
	printf("\n");
	printf("Free blocks count: %d\n\n", (int)(fs->blocks_count - occupied_blocks_count));
}

/*
 * List all files in the fat file system
 */
void list_files_fat_fs(const struct fat_file_system * fs)
{
	size_t i;
	printf("Content of the file system:\n");
	for(i = 0; i < fs->blocks_count; i++)
	{
		if(fs->inodes[i].first_file_block >= 0)
			printf("%s\n", fs->inodes[i].name);
	}
}

/*
 * Copy file to fat file system
 */
int copy_to_fat_fs(const struct fat_file_system * fs, const char * source_file_name, const char * dest_file_name)
{
	size_t required_blocks_count, i, free_block_index, old_index;
	int source_file_length;
	FILE * sFile;
	struct fat_inode * new_file_inode;
	char buffer[BLOCK_SIZE];
	int read_size;

	// open the source file
	sFile = fopen(source_file_name, "rb");
	if(sFile == NULL)
		return -1;
	// check if there is file with the same name in fat file system
	for(i = 0; i < fs->blocks_count; i++)
	{
		if(fs->inodes[i].first_file_block >=0 && strncmp(fs->inodes[i].name, dest_file_name, MAX_NAME_LENGTH) == 0)
			return -2;
	}
	fseek(sFile, 0, SEEK_END);
	source_file_length = ftell(sFile);
	fseek(sFile, 0, SEEK_SET);
	required_blocks_count = compute_required_blocks_count(source_file_length, BLOCK_SIZE);
	int a = get_free_blocks_count(fs);
	if(get_free_blocks_count(fs) < required_blocks_count)
		return -3;

	// find free i-node
	for(i = 0; i < fs->blocks_count; i++)
	{
		if(fs->inodes[i].first_file_block == -1)
		{
			new_file_inode = &(fs->inodes[i]);
			break;
		}
	}
	new_file_inode->file_size = source_file_length;
	strncpy(new_file_inode->name, dest_file_name, MAX_NAME_LENGTH);
	free_block_index = -1;
	old_index = free_block_index;
	if(source_file_length != 0)
	{
		int remaining_file_length = source_file_length;
		while(remaining_file_length > 0)
		{
			// find new free block
			for(i = free_block_index + 1; i < fs->blocks_count; i++)
			{
				if(fs->fat_table[i] == -2)
				{
					free_block_index = i;
					break;
				}
			}
			if(old_index != -1)
				fs->fat_table[old_index] = free_block_index;
			if(old_index == -1)
				new_file_inode->first_file_block = free_block_index;
			old_index = free_block_index;
			read_size = fread(buffer, 1, sizeof(buffer), sFile);
			int b = get_block_position_in_bytes(fs, free_block_index);
			int er;
			er = ferror(fs->pFile);
			fseek(fs->pFile, b, SEEK_SET);
			er = ferror(fs->pFile);
			int ab;
			ab = fwrite(buffer, 1, read_size, fs->pFile);
			er = ferror(fs->pFile);
			remaining_file_length -= BLOCK_SIZE;
		}
		fs->fat_table[old_index] = -1;
	}
	else
	{
		for(i = 0; i < fs->blocks_count; i++)
		{
			if(fs->fat_table[i] == -2)
			{
				free_block_index = i;
				break;
			}
		}
		new_file_inode->first_file_block = free_block_index;
		fs->fat_table[free_block_index] = -1;
	}
	fclose(sFile);
	return 0;
}

/*
 * Copy file from fat file system to linux
 */
int copy_from_fat_fs(const struct fat_file_system * fs, const char * source_file_name, const char * dest_file_name)
{
	FILE * dFile;
	size_t i, current_block, size_to_read;
	char buffer[BLOCK_SIZE];
	struct fat_inode * current_file_node;

	if(!(dFile = fopen(dest_file_name, "wb")))
		return -1;

	current_file_node = NULL;
	for(i = 0 ; i < fs->blocks_count; i++)
	{
		if(fs->inodes[i].first_file_block >= 0 && strncmp(fs->inodes[i].name, source_file_name, MAX_NAME_LENGTH) == 0)
		{
			current_file_node = &(fs->inodes[i]);
			break;
		}
	}
	if(current_file_node == NULL)
		return -2;

	// copy file block by block
	current_block = current_file_node->first_file_block;
	while(current_block != -1)
	{
		if(fs->fat_table[current_block] == -1)
			size_to_read = (int)((current_file_node->file_size)%BLOCK_SIZE);	// size to read for the last block
		else
			size_to_read = BLOCK_SIZE;
		fseek(fs->pFile, get_block_position_in_bytes(fs, current_block), SEEK_SET);
		// read current block
		if(fread(buffer, 1, size_to_read, fs->pFile) != size_to_read)
		{
			fclose(dFile);
			return -3;
		}
		fwrite(buffer, 1, size_to_read, dFile);
		current_block = fs->fat_table[current_block];
	}
	fclose(dFile);
	return 0;
}

/*
 * Delete fat file system
 */
void delete_file_fat_fs(const struct fat_file_system * fs, const char * file_name)
{
	int i, fat_index, new_fat_index;
	char * emptyName = "";
	for(i = 0; i < fs->blocks_count; i++)
	{
		// if file found, reset FAT table and clear i-node
		if(fs->inodes[i].first_file_block >= 0 && strncmp(fs->inodes[i].name, file_name, MAX_NAME_LENGTH) == 0)
		{
			fs->inodes[i].file_size = 0;
			strncpy(fs->inodes[i].name, emptyName, MAX_NAME_LENGTH);
			fat_index = fs->inodes[i].first_file_block;
			do
			{
				new_fat_index = fs->fat_table[fat_index];
				fs->fat_table[fat_index] = -2;
				fat_index = new_fat_index;
			}while(fat_index >= 0);
			fs->inodes[i].first_file_block = -1;
			return;
		}
	}
}

/*
 * Count free blocks in fat file system
 */
size_t get_free_blocks_count(const struct fat_file_system * fs)
{
	int i;
	size_t free_blocks = 0;
	for(i = 0; i < fs->blocks_count; i++)
	{
		if(fs->fat_table[i] == -2)
			free_blocks++;
	}
	return free_blocks;
}

/*
 * Count number of blocks to cover given space
 */
size_t block_count_from_space(const size_t total_space_size)
{
	/*
	 * each i-node has minimum 1 corresponding block and minimum one value
	 *  in fat table. Not all i-node must be utilized.
	 */
	return (total_space_size - sizeof(struct fat_superblock)) /
			(sizeof(struct fat_inode) + sizeof(size_t) + BLOCK_SIZE);
}

/*
 * Return byte-position of the given block in the file
 */
size_t get_block_position_in_bytes(const struct fat_file_system * fs, size_t block_number)
{
	return sizeof(struct fat_superblock) + sizeof(struct fat_inode) * fs->blocks_count +
			sizeof(fat_table_type) * fs->blocks_count + block_number * BLOCK_SIZE;
}

/*
 * Count number of blocks to be occupied by the new file
 */
int compute_required_blocks_count(const int file_size, const int block_size)
{
	int blocks_required = (size_t)(file_size/BLOCK_SIZE);
	if(file_size % block_size != 0 || file_size == 0)
		blocks_required++;
	return blocks_required;
}
