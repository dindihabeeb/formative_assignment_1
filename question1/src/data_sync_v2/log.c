#include "main.h"

static FILE *log_fp;

/**
 * open_log - opens the log file for appending
 * @path: path of the log file
 *
 * Return: 1 on success, 0 on failure
 */
int open_log(const char *path)
{
	log_fp = fopen(path, "a");
	if (!log_fp)
	{
		perror(path);
		return (0);
	}
	return (1);
}

/**
 * log_event - writes one timestamped line to the log file
 * @action: what happened, e.g. "NEW"
 * @name: the file or directory it happened to
 *
 * Return: nothing
 */
void log_event(const char *action, const char *name)
{
	char stamp[32];
	time_t now;
	struct tm *tm;

	if (!log_fp)
		return;
	now = time(NULL);
	tm = localtime(&now);
	if (!tm || strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", tm) == 0)
		strcpy(stamp, "unknown time");
	fprintf(log_fp, "[%s] %s %s\n", stamp, action, name);
}

/**
 * close_log - closes the log file if it is open
 *
 * Return: nothing
 */
void close_log(void)
{
	if (log_fp)
		fclose(log_fp);
	log_fp = NULL;
}
