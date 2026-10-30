#include "main.h"

static int log_fd = -1;

/**
 * log_open - opens (or creates) the log file for appending
 * @path: path of the log file
 *
 * Description: O_APPEND makes every write() go to the end of the file,
 * so the parent and the child can share the descriptor safely.
 *
 * Return: 1 on success, 0 on failure
 */
int log_open(const char *path)
{
	log_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (log_fd == -1)
	{
		fprintf(stderr, "FAIL open log file %s: %s\n", path,
			strerror(errno));
		return (0);
	}
	return (1);
}

/**
 * log_msg - prints a message and appends it, with a timestamp and the
 * process ID, to the log file using a single write()
 * @level: "OK", "FAIL" or "INFO"
 * @fmt: printf-style format of the message
 *
 * Return: nothing. errno is left unchanged.
 */
void log_msg(const char *level, const char *fmt, ...)
{
	char msg[MSG_LEN], line[MSG_LEN + 64], stamp[32];
	va_list ap;
	time_t now;
	struct tm tm;
	int len, saved_errno = errno;

	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	fprintf(strcmp(level, "FAIL") == 0 ? stderr : stdout, "%-4s %s\n",
		level, msg);

	if (log_fd != -1)
	{
		now = time(NULL);
		if (!localtime_r(&now, &tm) ||
		    !strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm))
			strcpy(stamp, "unknown time");
		len = snprintf(line, sizeof(line), "[%s] [pid %ld] %-4s %s\n",
			       stamp, (long)getpid(), level, msg);
		if (len > (int)sizeof(line) - 1)
			len = sizeof(line) - 1;
		if (len > 0 && write(log_fd, line, len) == -1)
			fprintf(stderr, "FAIL write log: %s\n",
				strerror(errno));
	}
	errno = saved_errno;
}

/**
 * log_close - closes the log file if it is open
 *
 * Return: nothing
 */
void log_close(void)
{
	if (log_fd != -1 && close(log_fd) == -1)
		fprintf(stderr, "FAIL close log: %s\n", strerror(errno));
	log_fd = -1;
}
