/*
 * copy_syscall.c
 * Version 1 of the file copy utility: uses the low-level system
 * calls read() and write() directly, moving CHUNK_SIZE bytes per
 * call. Every chunk therefore costs two real system calls.
 *
 * Usage:   ./copy_syscall <source> <destination>
 * Compile: gcc -Wall -Wextra -o copy_syscall copy_syscall.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define CHUNK_SIZE 512

int main(int argc, char **argv)
{
	int src_fd, dst_fd;
	ssize_t bytes_read;
	char buffer[CHUNK_SIZE];
	long long total = 0;
	struct timespec start, end;
	double elapsed;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <source> <destination>\n", argv[0]);
		return (EXIT_FAILURE);
	}

	src_fd = open(argv[1], O_RDONLY);
	if (src_fd < 0)
	{
		perror("open source");
		return (EXIT_FAILURE);
	}

	dst_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (dst_fd < 0)
	{
		perror("open destination");
		close(src_fd);
		return (EXIT_FAILURE);
	}

	clock_gettime(CLOCK_MONOTONIC, &start);

	while ((bytes_read = read(src_fd, buffer, CHUNK_SIZE)) > 0)
	{
		if (write(dst_fd, buffer, bytes_read) != bytes_read)
		{
			perror("write");
			close(src_fd);
			close(dst_fd);
			return (EXIT_FAILURE);
		}
		total += bytes_read;
	}

	clock_gettime(CLOCK_MONOTONIC, &end);

	close(src_fd);
	close(dst_fd);

	elapsed = (end.tv_sec - start.tv_sec) +
		  (end.tv_nsec - start.tv_nsec) / 1e9;
	printf("[syscall version] Copied %lld bytes in %.3f seconds "
	       "(%d-byte chunks)\n", total, elapsed, CHUNK_SIZE);

	return (EXIT_SUCCESS);
}
