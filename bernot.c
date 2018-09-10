#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef DELAY
#define DELAY 5
#endif

int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("%s: missing inputs\n", argv[0]);
		printf("Usage: %s file command\n", argv[0]);
		printf("where 'file' is the file to watch and 'command'\n");
		printf("is what is run when 'file' is changed.\n");
		return 3;
	}
	struct stat file_stat;
	if (stat(argv[1], &file_stat))
	{
		perror("stat");
		return 7;
	}
	time_t last = file_stat.st_mtime;

	while (1)
	{
		int err = stat(argv[1], &file_stat);
		if (err != 0)
		{
			perror("stat");
		}
		time_t current = file_stat.st_mtime;

		if (last != current)
		{
			last = current;
			system(argv[2]);
		}

		sleep(DELAY);
	}

	return 0;
}
