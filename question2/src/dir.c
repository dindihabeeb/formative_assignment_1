#include "main.h"

/**
 * ensure_dir - creates the backup directory if it does not exist yet
 * @path: path of the backup directory
 *
 * Description: mkdir() is tried first. If it fails with EEXIST, stat()
 * checks that the existing path really is a directory.
 *
 * Return: 1 if the directory is ready, 0 on failure
 */
int ensure_dir(const char *path)
{
	struct stat st;

	if (mkdir(path, 0755) == 0)
	{
		log_msg("OK", "created backup directory %s", path);
		return (1);
	}
	if (errno != EEXIST)
	{
		log_msg("FAIL", "mkdir %s: %s", path, strerror(errno));
		return (0);
	}
	if (stat(path, &st) == -1)
	{
		log_msg("FAIL", "stat %s: %s", path, strerror(errno));
		return (0);
	}
	if (!S_ISDIR(st.st_mode))
	{
		log_msg("FAIL", "%s exists but is not a directory", path);
		return (0);
	}
	log_msg("OK", "backup directory %s already exists", path);
	return (1);
}

/**
 * build_dest_path - builds "<dir>/<file name of src>"
 * @buf: buffer for the result
 * @size: size of the buffer
 * @dir: backup directory
 * @src: source file path
 *
 * Return: 1 on success, 0 if the path is too long
 */
int build_dest_path(char *buf, size_t size, const char *dir,
		    const char *src)
{
	const char *name = strrchr(src, '/');
	int n;

	name = name ? name + 1 : src;
	n = snprintf(buf, size, "%s/%s", dir, name);
	if (n < 0 || (size_t)n >= size)
	{
		log_msg("FAIL", "destination path for %s is too long", src);
		return (0);
	}
	return (1);
}
