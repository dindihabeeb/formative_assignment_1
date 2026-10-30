#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define DEFAULT_SOURCE "source.txt"
#define DEFAULT_BACKUP_DIR "backup"
#define LOG_FILE "backup_tool.log"
#define PATH_LEN 1024
#define BUF_SIZE 4096
#define MSG_LEN 512

/* Exit codes the copy child uses to tell the parent what went wrong */
#define COPY_OK 0
#define COPY_ERR_SOURCE 2
#define COPY_ERR_DEST 3
#define COPY_ERR_READ 4
#define COPY_ERR_WRITE 5
#define COPY_ERR_MEMORY 6
#define COPY_ERR_CLOSE 7

/* log.c */
int log_open(const char *path);
void log_msg(const char *level, const char *fmt, ...);
void log_close(void);

/* dir.c */
int ensure_dir(const char *path);
int build_dest_path(char *buf, size_t size, const char *dir,
		    const char *src);

/* copy.c */
int copy_file(const char *src, const char *dst);
const char *copy_error_str(int code);

#endif /* MAIN_H */
