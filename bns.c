#include "inppar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef DELAY
#define DELAY 1
#endif

typedef struct bnss
{
	time_t last;
	char* fn;
	struct bnss* next;
} node;

int main(int argc, char** argv)
{
	int i, err, cnt;
	struct stat file_stat;
	int ignore_access_errors = 0;
	int print_all_file_names = 0;
	int wait_time = DELAY;
	int background = 0;

	if (argc < 3)
	{
		printf("%s: missing inputs\n", argv[0]);
		printf("Usage: %s file command [FLAGS]\n", argv[0]);
		printf("where 'file' is a file including all\n");
		printf("the names of the files to watch and 'command'\n");
		printf("is what is run when 'file' is changed.\n");
		printf("\n");
		printf("Flags:\n");
		printf("  -c         Ignores all files that can't be accessed.\n");
		printf("  -p         Prints out which files are being watched.\n");
		printf("  -d<num>    Checks fiels for updates every <num> seconds.\n");
		printf("  -b         Executes in the background.\n");
		return 3;
	}

	FILE* fp = fopen(argv[1], "r");
	if (fp == NULL)
	{
		printf("%s: file not found: %s\n", argv[0], argv[1]);
		return 2;
	}

	inppar_fetchb(argc, argv, "-p", &print_all_file_names);
	inppar_fetchb(argc, argv, "-c", &ignore_access_errors);
	inppar_fetchi(argc, argv, "-d", &wait_time);
	inppar_fetchb(argc, argv, "-b", &background);

	char* buffer = malloc(sizeof(char)*1024);

	node* head = NULL;
	node* body = head;

	char c = fgetc(fp);
	i = 0;
	cnt = 0;
	while (c != EOF)
	{
		buffer[i++] = c;
		assert(i < 1024);

		c = fgetc(fp);
		if (c == 13 || c == 10)
		{
			buffer[i] = '\0';
			if (head == NULL)
			{
				head = malloc(sizeof(node*));
				head->next = NULL;
				body = head;
			}
			else
			{
				body->next = malloc(sizeof(node*));
				body = body->next;
				body->next = NULL;
			}
			cnt++;
			body->fn = malloc(sizeof(char)*i);
			strcpy(body->fn, buffer);
			c = fgetc(fp);
			i = 0;
			while (c == 13 || c == 10)
			{
				c = fgetc(fp);
			}
		}
	}
	body = head;

	free(buffer);

	while (head != NULL && access(head->fn, F_OK) != 0)
	{
		printf("Error accessing file '%s'.\n", head->fn);
		if (ignore_access_errors == 0)
		{
			printf("Ignore this file [Y/n] ");
			c = getchar();
			if (c != 10)
			{
				while (getchar() != 10);
			}

			if (c == 'Y' || c == 'y' || c == 10)
			{
				node* tmp = head-> next;
				free(head->fn);
				free(head);
				head = tmp;
				cnt--;
			}
			else
			{
				while (head != NULL)
				{
					free(head->fn);
					node* tmp = head->next;
					free(head);
					head = tmp;
				}

				return 13;
			}
		}
		else
		{
			node* tmp = head-> next;
			free(head->fn);
			free(head);
			head = tmp;
			cnt--;
		}
	}
	if (head != NULL)
	{
		body = head;
		while (body->next != NULL)
		{
			if (access(body->next->fn, F_OK) != 0)
			{
				printf("Error accessing file '%s'.\n", body->next->fn);
				if (ignore_access_errors == 0)
				{
					printf("Ignore this file [Y/n] ");
					c = getchar();
					if (c != 10)
					{
						while (getchar() != 10);
					}

					if (c == 'Y' || c == 'y' || c == 10)
					{
						cnt--;
						free(body->next->fn);
						body->next = body->next->next;
					}
					else
					{
						while (head != NULL)
						{
							free(head->fn);
							node* tmp = head->next;
							free(head);
							head = tmp;
						}

						return 13;
					}
				}
				else
				{
					cnt--;
					free(body->next->fn);
					body->next = body->next->next;
				}
			}
			else
			{
				if (stat(body->fn, &file_stat))
				{
					perror("stat");
					return 8;
				}

				body->last = file_stat.st_mtime;
				body = body->next;
			}
		}
	}
	if (stat(body->fn, &file_stat))
	{
		perror("stat");
		return 8;
	}

	body->last = file_stat.st_mtime;

	printf("bns is running on %d files", cnt);
	if (print_all_file_names == 1)
	{
		printf(":\n");
		body = head;
		while (body != NULL)
		{
			printf("%s\n", body->fn);
			body = body->next;
		}
	}
	else
	{
		printf(".\n");
	}

	if (background == 1)
	{
		pid_t pid = fork();
		if (pid != 0)
		{
			printf("Forked to pid [%d]\n", pid);
			return 0;
		}
	}

#define printf(...) if(background == 0) printf(__VA_ARGS__)

	while (1)
	{
		body = head;
		while (body != NULL)
		{
			err = stat(body->fn, &file_stat);
			if (background == 0 && err != 0)
			{
				perror("stat");
			}
			time_t current = file_stat.st_mtime;

			if (body->last != current)
			{
				break;
			}
			body = body->next;
		}
		if (body != NULL)
		{
			body = head;
			while (body != NULL)
			{
				err = stat(body->fn, &file_stat);
				if (background == 0 && err != 0)
				{
					perror("stat");
				}

				body->last = file_stat.st_mtime;
				body = body->next;
			}

			system(argv[2]);
		}

		sleep(wait_time);
	}

	while (head != NULL)
	{
		free(head->fn);
		node* tmp = head->next;
		free(head);
		head = tmp;
	}

	return 0;
}
