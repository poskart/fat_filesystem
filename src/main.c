#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fat_filesystem.h"

int main(int ArgC, char ** ArgV)
{
	char * fat_fs_name;
	char * command;
	struct fat_file_system * v;

	if(ArgC < 3)
	{
		printf("%s <fat_fs_name name> command (...)\n", ArgV[0]);
		printf(
			"Availible commands: \n"
			"- create <size in bytes>\n"
			"- dump\n"
			"- list\n"
			"- push <source file name> <destination file name>\n"
			"- pull <source file name> <destination file name>\n"
			"- remove <file name>\n"
			"- delete\n"
		);
		return 1;
	}

	fat_fs_name = ArgV[1];
	command = ArgV[2];

	if(strcmp("create", command) == 0)
	{
		if(ArgC == 4)
		{
			size_t size = atoi(ArgV[3]);
			v = create_fat_fs(fat_fs_name, size);
			if(!v)
			{
				printf("Error: cannot create virtual file system.\n");
				return 2;
			}
			unmount_fat_fs(v);
		}
		else
			printf("%s <fat_fs_name name> create <size in bytes>\n", ArgV[0]);
	}
	else if(strcmp("dump", command) == 0)
	{
		if(ArgC == 3)
		{
			v = mount_fat_fs(fat_fs_name);
			if(!v)
			{
				printf("Error: cannot open virtual file system.\n");
				return 2;
			}

			dump_fat_fs(v);

			unmount_fat_fs(v);
		}
		else
			printf("%s <fat_fs_name name> dump\n", ArgV[0]);
	}
	else if(strcmp("list", command) == 0)
	{
		if(ArgC == 3)
		{
			v = mount_fat_fs(fat_fs_name);
			if(!v)
			{
				printf("Error: cannot open virtual file system.\n");
				return 2;
			}

			list_files_fat_fs(v);

			unmount_fat_fs(v);
		}
		else
			printf("%s <fat_fs_name name> list\n", ArgV[0]);
	}
	else if(strcmp("push", command) == 0)
	{
		if(ArgC == 5)
		{
			v = mount_fat_fs(fat_fs_name);
			if(!v)
			{
				printf("Error: cannot open virtual file system.\n");
				return 2;
			}

			printf("Pushing new file, result: %d\n", copy_to_fat_fs(v, ArgV[3], ArgV[4]));

			unmount_fat_fs(v);
		}
		else
			printf("%s <fat_fs_name name> push <source file name> <destination file name>\n", ArgV[0]);
	}
	else if(strcmp("pull", command) == 0)
	{
		if(ArgC == 5)
		{
			v = mount_fat_fs(fat_fs_name);
			if(!v)
			{
				printf("Error: cannot open virtual file system.\n");
				return 2;
			}

			printf("Downloading file, result: %d\n", copy_from_fat_fs(v, ArgV[3], ArgV[4]));

			unmount_fat_fs(v);
		}
		else
			printf("%s <fat_fs_name name> pull <source file name> <destination file name>\n", ArgV[0]);
	}
	else if(strcmp("remove", command) == 0)
	{
		if(ArgC == 4)
		{
			v = mount_fat_fs(fat_fs_name);
			if(!v)
			{
				printf("Error: cannot open virtual file system.\n");
				return 2;
			}

			delete_file_fat_fs(v, ArgV[3]);

			unmount_fat_fs(v);
		}
		else
			printf("%s <fat_fs_name name> pull <source file name> <destination file name>\n", ArgV[0]);
	}
	else if(strcmp("delete", command) == 0)
	{
		if(ArgC == 3)
		{
			delete_fat_fs(fat_fs_name);
		}
		else
			printf("%s <fat_fs_name name> delete\n", ArgV[0]);
	}
	else
	{
		printf("%s invalid command `%s`\n", ArgV[0], command);
		return 1;
	}


	return 0;
}
