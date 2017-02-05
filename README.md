# fat_filesystem
This is a simple fat-like file system written in C. Developed for UNIX OS (Minix, Linux).

1. File system is based on FAT table, with each element corresponding to the specific data block. 
2. Each file has exactly one i-node, and data blocks.
3. The virtual file system file consisis of 3 parts:
	- superblock, containing info about i-nodes list size ( = block list size = FAT table size), file system size, free blocks count.
	- FAT table (N)
	- data blocks (N)
4. Every single number in FAT table is an index to the next data block in the file. 
5. The last index in FAT table which corresponds to the last data block in the file has value -1.
6. Default value (block not assigned to file) of the FAT table element is -2.
7. I-node contains:
	- file name (max 24 characters)
	- file size
	- first FAT table index (index to the first element in FAT table - first data block)


