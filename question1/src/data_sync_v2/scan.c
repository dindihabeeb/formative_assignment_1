#include "main.h"

/**
 * join_path - builds "dir/name" into a buffer
 * @buf: buffer to write the path into
 * @size: size of the buffer
 * @dir: directory part
 * @name: file name part
 *
 * Return: 1 on success, 0 if the path did not fit in the buffer
 */
int join_path(char *buf, size_t size, const char *dir, const char *name)
{
	int n = snprintf(buf, size, "%s/%s", dir, name);

	return (n >= 0 && (size_t)n < size);
}

/**
 * scan_dir - reads the names of the regular files in a directory,
 * using stat() to skip sub-directories and other special files
 * @path: path of the directory to read
 * @list: list to store the names in
 *
 * Return: 1 on success, 0 if the directory could not be opened
 */
int scan_dir(const char *path, file_list_t *list)
{
	DIR *dir;
	struct dirent *entry;
	struct stat st;
	char full[PATH_LEN];

	list->count = 0;
	dir = opendir(path);
	if (!dir)
	{
		perror(path);
		return (0);
	}

	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0)
			continue;
		if (!join_path(full, sizeof(full), path, entry->d_name) ||
		    stat(full, &st) == -1 || !S_ISREG(st.st_mode))
			continue;
		if (list->count >= MAX_FILES)
		{
			fprintf(stderr, "Warning: more than %d files in %s, "
				"the rest are ignored\n", MAX_FILES, path);
			break;
		}
		snprintf(list->names[list->count], NAME_LEN, "%s", entry->d_name);
		list->count++;
	}
	closedir(dir);
	return (1);
}

/**
 * has_name - checks if a name is in a list
 * @list: list to search
 * @name: name to look for
 *
 * Return: 1 if the name is in the list, 0 otherwise
 */
int has_name(const file_list_t *list, const char *name)
{
	int i;

	for (i = 0; i < list->count; i++)
		if (strcmp(list->names[i], name) == 0)
			return (1);
	return (0);
}
