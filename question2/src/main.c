#include "main.h"

/**
 * run_copy_in_child - forks a child that copies the file, then waits for it
 * @src: source file path
 * @dst: destination file path
 *
 * Description: stdout and stderr are flushed before fork() so the child
 * does not inherit (and print again) unwritten output.
 *
 * Return: 1 if the child copied the file, 0 otherwise
 */
static int run_copy_in_child(const char *src, const char *dst)
{
	pid_t pid, w;
	int status, code;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid == -1)
	{
		log_msg("FAIL", "fork: %s", strerror(errno));
		return (0);
	}
	if (pid == 0)
	{
		code = copy_file(src, dst);
		log_close();
		exit(code);
	}

	log_msg("INFO", "child %ld started to copy the file", (long)pid);
	do {
		w = waitpid(pid, &status, 0);
	} while (w == -1 && errno == EINTR);
	if (w == -1)
	{
		log_msg("FAIL", "waitpid: %s", strerror(errno));
		return (0);
	}
	if (WIFEXITED(status) && WEXITSTATUS(status) == COPY_OK)
	{
		log_msg("OK", "child %ld finished: copy succeeded", (long)pid);
		return (1);
	}
	if (WIFEXITED(status))
	{
		code = WEXITSTATUS(status);
		log_msg("FAIL", "child %ld exited with code %d: %s", (long)pid,
			code, copy_error_str(code));
	}
	else if (WIFSIGNALED(status))
		log_msg("FAIL", "child %ld was killed by signal %d", (long)pid,
			WTERMSIG(status));
	return (0);
}

/**
 * verify_copy - checks with stat() that the copy has the source's size
 * @src: source file path
 * @dst: destination file path
 *
 * Return: 1 if the sizes match, 0 otherwise
 */
static int verify_copy(const char *src, const char *dst)
{
	struct stat s, d;

	if (stat(src, &s) == -1 || stat(dst, &d) == -1)
	{
		log_msg("FAIL", "verify: stat: %s", strerror(errno));
		return (0);
	}
	if (s.st_size != d.st_size)
	{
		log_msg("FAIL", "verify: source is %ld bytes, copy is %ld",
			(long)s.st_size, (long)d.st_size);
		return (0);
	}
	log_msg("OK", "verified %s (%ld bytes)", dst, (long)d.st_size);
	return (1);
}

/**
 * main - Entry point, backs up one file into a backup directory
 * @argc: number of arguments
 * @argv: [source_file] [backup_dir], defaults are source.txt and backup
 *
 * Return: does not return, exits with EXIT_SUCCESS or EXIT_FAILURE
 */
int main(int argc, char *argv[])
{
	const char *src = argc > 1 ? argv[1] : DEFAULT_SOURCE;
	const char *dir = argc > 2 ? argv[2] : DEFAULT_BACKUP_DIR;
	char dst[PATH_LEN];
	int ok;

	if (argc > 3)
	{
		fprintf(stderr, "Usage: %s [source_file] [backup_dir]\n",
			argv[0]);
		exit(EXIT_FAILURE);
	}
	if (!log_open(LOG_FILE))
		exit(EXIT_FAILURE);

	log_msg("INFO", "backup started: %s -> %s/", src, dir);
	ok = ensure_dir(dir) && build_dest_path(dst, sizeof(dst), dir, src) &&
	     run_copy_in_child(src, dst) && verify_copy(src, dst);
	if (ok)
		log_msg("OK", "backup finished");
	else
		log_msg("FAIL", "backup failed");
	log_close();
	exit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
