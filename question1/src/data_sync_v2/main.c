#include "main.h"

/**
 * main - Entry point, compares a source directory with a backup directory
 * using names, file metadata and checksums, and logs every result
 * @argc: number of arguments
 * @argv: argv[1] is the source directory, argv[2] is the backup directory
 *
 * Return: EXIT_SUCCESS, or EXIT_FAILURE on bad usage or a directory error
 */
int main(int argc, char *argv[])
{
	static file_list_t src, dst;
	sync_state_t state;
	int i, to_sync = 0, errors = 0;
	char summary[64];

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <source_dir> <backup_dir>\n", argv[0]);
		return (EXIT_FAILURE);
	}

	printf("data_sync %s - comparing %s with %s\n",
	       VERSION, argv[1], argv[2]);
	if (!open_log(LOG_FILE))
		fprintf(stderr, "Warning: running without a log file\n");
	log_event("START", argv[1]);
	if (!scan_dir(argv[1], &src) || !scan_dir(argv[2], &dst))
	{
		log_event("FAILED", "could not read a directory");
		close_log();
		return (EXIT_FAILURE);
	}

	for (i = 0; i < src.count; i++)
	{
		if (!has_name(&dst, src.names[i]))
			state = STATE_NEW;
		else
			state = check_file(argv[1], argv[2], src.names[i]);

		if (state == STATE_ERROR)
			errors++;
		else if (state != STATE_SAME)
			to_sync++;
		printf("  %-18s  %s\n", state_str(state), src.names[i]);
		log_event(state_str(state), src.names[i]);
	}

	printf("%d of %d file(s) need to be synced, %d error(s).\n",
	       to_sync, src.count, errors);
	snprintf(summary, sizeof(summary), "%d to sync, %d error(s)",
		 to_sync, errors);
	log_event("DONE", summary);
	close_log();
	return (EXIT_SUCCESS);
}
