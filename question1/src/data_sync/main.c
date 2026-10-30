#include "main.h"

/**
 * main - Entry point, lists the files in the source directory that are
 * missing from the backup directory
 * @argc: number of arguments
 * @argv: argv[1] is the source directory, argv[2] is the backup directory
 *
 * Return: EXIT_SUCCESS, or EXIT_FAILURE on bad usage or a directory error
 */
int main(int argc, char *argv[])
{
	static file_list_t src, dst;
	int i, missing = 0;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <source_dir> <backup_dir>\n", argv[0]);
		return (EXIT_FAILURE);
	}

	printf("data_sync %s - comparing %s with %s\n",
	       VERSION, argv[1], argv[2]);
	if (!scan_dir(argv[1], &src) || !scan_dir(argv[2], &dst))
		return (EXIT_FAILURE);

	for (i = 0; i < src.count; i++)
	{
		if (!has_name(&dst, src.names[i]))
		{
			printf("  NEEDS SYNC  %s\n", src.names[i]);
			missing++;
		}
	}

	printf("%d of %d entries need to be synced.\n", missing, src.count);
	return (EXIT_SUCCESS);
}
