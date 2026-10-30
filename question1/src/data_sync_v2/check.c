#include "main.h"

/**
 * check_file - decides if a source file needs to be copied to the backup
 * @src_dir: source directory
 * @dst_dir: backup directory
 * @name: name of the file to check
 *
 * Description: the cheap metadata checks run first. Different sizes mean
 * the file changed. Same size and same modification time means the file
 * is taken as unchanged. Only when the size matches but the time does not
 * are both files read and their checksums compared.
 *
 * Return: the sync state of the file
 */
sync_state_t check_file(const char *src_dir, const char *dst_dir,
			const char *name)
{
	char src_path[PATH_LEN], dst_path[PATH_LEN];
	struct stat src_st, dst_st;
	unsigned long src_sum, dst_sum;

	if (!join_path(src_path, sizeof(src_path), src_dir, name) ||
	    !join_path(dst_path, sizeof(dst_path), dst_dir, name))
		return (STATE_ERROR);
	if (stat(src_path, &src_st) == -1)
		return (STATE_ERROR);
	if (stat(dst_path, &dst_st) == -1)
		return (errno == ENOENT ? STATE_NEW : STATE_ERROR);
	if (!S_ISREG(dst_st.st_mode))
		return (STATE_ERROR);

	if (src_st.st_size != dst_st.st_size)
		return (STATE_SIZE);
	if (src_st.st_mtime == dst_st.st_mtime)
		return (STATE_SAME);

	if (!file_checksum(src_path, &src_sum) ||
	    !file_checksum(dst_path, &dst_sum))
		return (STATE_ERROR);
	return (src_sum == dst_sum ? STATE_SAME : STATE_CONTENT);
}

/**
 * state_str - gives the text label for a sync state
 * @state: the state
 *
 * Return: a string that must not be freed
 */
const char *state_str(sync_state_t state)
{
	switch (state)
	{
	case STATE_SAME:
		return ("UP TO DATE");
	case STATE_NEW:
		return ("NEW");
	case STATE_SIZE:
		return ("CHANGED (size)");
	case STATE_CONTENT:
		return ("CHANGED (checksum)");
	default:
		return ("ERROR");
	}
}
