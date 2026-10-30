#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#define VERSION "2.0"
#define MAX_FILES 256
#define NAME_LEN 256
#define PATH_LEN 1024
#define BUF_SIZE 4096
#define LOG_FILE "data_sync.log"

/**
 * struct file_list_s - The names of the regular files in one directory
 *
 * @names: The file names
 * @count: Number of names stored, never more than MAX_FILES
 */
typedef struct file_list_s
{
	char	names[MAX_FILES][NAME_LEN];
	int	count;
} file_list_t;

/**
 * enum sync_state - What needs to happen to one source file
 *
 * @STATE_SAME: The backup copy is up to date
 * @STATE_NEW: There is no backup copy yet
 * @STATE_SIZE: The backup copy has a different size
 * @STATE_CONTENT: Same size, but the checksums are different
 * @STATE_ERROR: The file could not be checked
 */
typedef enum sync_state
{
	STATE_SAME,
	STATE_NEW,
	STATE_SIZE,
	STATE_CONTENT,
	STATE_ERROR
} sync_state_t;

/* scan.c */
int join_path(char *buf, size_t size, const char *dir, const char *name);
int scan_dir(const char *path, file_list_t *list);
int has_name(const file_list_t *list, const char *name);

/* check.c */
sync_state_t check_file(const char *src_dir, const char *dst_dir,
			const char *name);
const char *state_str(sync_state_t state);

/* checksum.c */
int file_checksum(const char *path, unsigned long *sum);

/* log.c */
int open_log(const char *path);
void log_event(const char *action, const char *name);
void close_log(void);

#endif /* MAIN_H */
