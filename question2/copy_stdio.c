/*
 * copy_stdio.c
 * Version 2 of the file copy utility: uses the standard I/O
 * library functions fread() and fwrite() with the same
 * CHUNK_SIZE as version 1. The stdio library maintains its own
 * internal buffer, so many fread/fwrite calls are served from
 * memory and only occasionally trigger a real system call.
 *
 * Usage:   ./copy_stdio <source> <destination>
 * Compile: gcc -Wall -Wextra -o copy_stdio copy_stdio.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define CHUNK_SIZE 512
int main(int argc, char **argv)
{
	FILE *src, *dst;
	size_t bytes_read;
	char buffer[CHUNK_SIZE];
	long long total = 0;
	struct timespec start, end;
	double elapsed;
	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
		return (EXIT_FAILURE);
	}
	src = fopen(argv[1], "rb");
	if (src == NULL)
	{
		perror("fopen source");
		return (EXIT_FAILURE);
	}
	dst = fopen(argv[2], "wb");
	if (dst == NULL)
	{
		perror("fopen destination");
		fclose(src);
		return (EXIT_FAILURE);
	}
	clock_gettime(CLOCK_MONOTONIC, &start);
	while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, src)) > 0)
	{
		if (fwrite(buffer, 1, bytes_read, dst) != bytes_read)
		{
			perror("fwrite");
			fclose(src);
			fclose(dst);
			return (EXIT_FAILURE);
		}
		total += (long long)bytes_read;
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	if (ferror(src))
	{
		perror("fread");
		fclose(src);
		fclose(dst);
		return (EXIT_FAILURE);
	}
	fclose(src);
	fclose(dst);
	elapsed = (end.tv_sec - start.tv_sec) +
		  (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("[stdio version] Copied %lld bytes in %.3f seconds "
	       "(%d-byte chunks)\n", total, elapsed, CHUNK_SIZE);
	return (EXIT_SUCCESS);
}
