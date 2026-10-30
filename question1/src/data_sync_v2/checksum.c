#include "main.h"

#define ADLER_MOD 65521UL

/**
 * file_checksum - works out the Adler-32 checksum of a file's contents
 * @path: path of the file to read
 * @sum: where to store the checksum
 *
 * Description: Adler-32 keeps two running sums. a is 1 plus the sum of
 * all bytes, b is the sum of every value a has taken. Both are kept
 * below 65521 and the result is (b << 16) | a.
 *
 * Return: 1 on success, 0 if the file could not be read
 */
int file_checksum(const char *path, unsigned long *sum)
{
	FILE *fp;
	unsigned char buf[BUF_SIZE];
	unsigned long a = 1, b = 0;
	size_t n, i;
	int ok;

	fp = fopen(path, "rb");
	if (!fp)
	{
		perror(path);
		return (0);
	}

	while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
	{
		for (i = 0; i < n; i++)
		{
			a = (a + buf[i]) % ADLER_MOD;
			b = (b + a) % ADLER_MOD;
		}
	}
	ok = !ferror(fp);
	fclose(fp);
	if (!ok)
		return (0);

	*sum = (b << 16) | a;
	return (1);
}
