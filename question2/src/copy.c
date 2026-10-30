#include "main.h"

/**
 * write_all - writes a whole buffer, retrying after short writes
 * @fd: file descriptor to write to
 * @buf: data to write
 * @len: number of bytes to write
 *
 * Return: 1 on success, 0 on error (errno is set by write)
 */
static int write_all(int fd, const char *buf, size_t len)
{
	ssize_t n;

	while (len > 0)
	{
		n = write(fd, buf, len);
		if (n == -1)
		{
			if (errno == EINTR)
				continue;
			return (0);
		}
		buf += n;
		len -= n;
	}
	return (1);
}

/**
 * copy_data - copies everything from one descriptor to another
 * @in_fd: descriptor to read from
 * @out_fd: descriptor to write to
 * @buf: work buffer of BUF_SIZE bytes
 *
 * Return: COPY_OK, COPY_ERR_READ or COPY_ERR_WRITE
 */
static int copy_data(int in_fd, int out_fd, char *buf)
{
	ssize_t n;
	long total = 0;

	for (;;)
	{
		n = read(in_fd, buf, BUF_SIZE);
		if (n == 0)
			break;
		if (n == -1)
		{
			if (errno == EINTR)
				continue;
			log_msg("FAIL", "read source: %s", strerror(errno));
			return (COPY_ERR_READ);
		}
		if (!write_all(out_fd, buf, n))
		{
			log_msg("FAIL", "write destination: %s",
				strerror(errno));
			return (COPY_ERR_WRITE);
		}
		total += n;
	}
	log_msg("OK", "copied %ld bytes", total);
	return (COPY_OK);
}

/**
 * copy_file - opens the source, creates the destination and copies
 * @src: source file path
 * @dst: destination file path
 *
 * Description: every descriptor that is opened is closed and the buffer
 * is freed on every return path.
 *
 * Return: COPY_OK or one of the COPY_ERR_* codes
 */
int copy_file(const char *src, const char *dst)
{
	int in_fd, out_fd, code;
	char *buf;

	in_fd = open(src, O_RDONLY);
	if (in_fd == -1)
	{
		log_msg("FAIL", "open source %s: %s", src, strerror(errno));
		return (COPY_ERR_SOURCE);
	}
	log_msg("OK", "opened source %s", src);
	out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out_fd == -1)
	{
		log_msg("FAIL", "create destination %s: %s", dst,
			strerror(errno));
		close(in_fd);
		return (COPY_ERR_DEST);
	}
	log_msg("OK", "created destination %s", dst);

	buf = malloc(BUF_SIZE);
	code = buf ? copy_data(in_fd, out_fd, buf) : COPY_ERR_MEMORY;
	if (!buf)
		log_msg("FAIL", "malloc: %s", strerror(errno));
	free(buf);
	close(in_fd);
	if (close(out_fd) == -1 && code == COPY_OK)
	{
		log_msg("FAIL", "close destination: %s", strerror(errno));
		code = COPY_ERR_CLOSE;
	}
	return (code);
}

/**
 * copy_error_str - describes a copy exit code
 * @code: the code returned by copy_file()
 *
 * Return: a string that must not be freed
 */
const char *copy_error_str(int code)
{
	switch (code)
	{
	case COPY_OK:
		return ("success");
	case COPY_ERR_SOURCE:
		return ("could not open the source file");
	case COPY_ERR_DEST:
		return ("could not create the destination file");
	case COPY_ERR_READ:
		return ("read error");
	case COPY_ERR_WRITE:
		return ("write error");
	case COPY_ERR_MEMORY:
		return ("out of memory");
	case COPY_ERR_CLOSE:
		return ("could not close the destination file");
	default:
		return ("unknown error");
	}
}
