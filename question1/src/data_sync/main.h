#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define VERSION "1.0"
#define MAX_FILES 256
#define NAME_LEN 256

/**
 * struct file_list_s - The names of the files found in one directory
 *
 * @names: The file names
 * @count: Number of names stored, never more than MAX_FILES
 */
typedef struct file_list_s
{
	char	names[MAX_FILES][NAME_LEN];
	int	count;
} file_list_t;

/* scan.c */
int scan_dir(const char *path, file_list_t *list);
int has_name(const file_list_t *list, const char *name);

#endif /* MAIN_H */
