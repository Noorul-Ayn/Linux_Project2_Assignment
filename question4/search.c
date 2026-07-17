/*
 * search.c
 * Multi-threaded keyword search across several text files.
 * Each thread takes one file at a time from a shared work list,
 * counts how many times the keyword appears in it, and writes the
 * result into one shared output file. A mutex protects the shared
 * work index so no file is processed twice, and a second mutex
 * protects every write to the shared output file and the running
 * total so concurrent writes cannot corrupt each other.
 *
 * Compile: gcc -Wall -Wextra -pthread -o search search.c
 * Run:     ./search keyword output.txt file1.txt file2.txt ... <threads>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define CHUNK 65536

/* State shared by every worker thread */
typedef struct shared_s
{
	const char *keyword;
	size_t keyword_len;
	char **files;
	int num_files;
	int next_index;
	long total;
	FILE *out;
	pthread_mutex_t work_lock;
	pthread_mutex_t out_lock;
} shared_t;

/*
 * read_file - read an entire file into memory using fread
 * Grows the buffer as needed. Returns the buffer and sets *out_len,
 * or NULL on failure. Caller frees the buffer.
 */
static char *read_file(const char *path, size_t *out_len)
{
	FILE *f = fopen(path, "rb");
	char *buf, *tmp;
	size_t cap = CHUNK, len = 0, n;

	if (f == NULL)
		return (NULL);
	buf = malloc(cap);
	if (buf == NULL)
	{
		fclose(f);
		return (NULL);
	}
	for (;;)
	{
		if (len == cap)
		{
			cap *= 2;
			tmp = realloc(buf, cap);
			if (tmp == NULL)
			{
				free(buf);
				fclose(f);
				return (NULL);
			}
			buf = tmp;
		}
		n = fread(buf + len, 1, cap - len, f);
		if (n == 0)
			break;
		len += n;
	}
	if (ferror(f))
	{
		free(buf);
		fclose(f);
		return (NULL);
	}
	fclose(f);
	*out_len = len;
	return (buf);
}

/*
 * count_occurrences - count non-overlapping matches of kw in buf
 */
static long count_occurrences(const char *buf, size_t len,
			      const char *kw, size_t klen)
{
	long count = 0;
	size_t i = 0;

	if (klen == 0 || len < klen)
		return (0);
	while (i + klen <= len)
	{
		if (memcmp(buf + i, kw, klen) == 0)
		{
			count++;
			i += klen;
		}
		else
		{
			i++;
		}
	}
	return (count);
}

/*
 * worker - thread routine
 * Repeatedly claims the next unprocessed file under work_lock,
 * counts the keyword in it, then writes the result and updates the
 * shared total under out_lock.
 */
static void *worker(void *arg)
{
	shared_t *s = (shared_t *)arg;
	int idx;
	char *buf;
	size_t len = 0;
	long occ;

	for (;;)
	{
		pthread_mutex_lock(&s->work_lock);
		idx = s->next_index;
		if (idx >= s->num_files)
		{
			pthread_mutex_unlock(&s->work_lock);
			break;
		}
		s->next_index++;
		pthread_mutex_unlock(&s->work_lock);

		buf = read_file(s->files[idx], &len);
		if (buf == NULL)
		{
			pthread_mutex_lock(&s->out_lock);
			fprintf(s->out, "%s: could not be opened\n",
				s->files[idx]);
			pthread_mutex_unlock(&s->out_lock);
			continue;
		}
		occ = count_occurrences(buf, len, s->keyword,
					s->keyword_len);
		free(buf);
		pthread_mutex_lock(&s->out_lock);
		fprintf(s->out, "%s: %ld occurrences\n", s->files[idx], occ);
		s->total += occ;
		pthread_mutex_unlock(&s->out_lock);
	}
	return (NULL);
}

int main(int argc, char **argv)
{
	shared_t s;
	pthread_t *threads;
	struct timespec start, end;
	double elapsed;
	int requested, num_threads, i;

	if (argc < 5)
	{
		fprintf(stderr, "Usage: %s keyword output.txt file1 "
			"file2 ... <threads>\n", argv[0]);
		return (EXIT_FAILURE);
	}
	s.keyword = argv[1];
	s.keyword_len = strlen(argv[1]);
	s.files = &argv[3];
	s.num_files = argc - 4;
	s.next_index = 0;
	s.total = 0;
	requested = atoi(argv[argc - 1]);
	if (requested < 1)
	{
		fprintf(stderr, "Thread count must be at least 1\n");
		return (EXIT_FAILURE);
	}
	num_threads = requested;
	if (num_threads > s.num_files)
	{
		fprintf(stderr, "Note: more threads than files, using %d "
			"(one per file)\n", s.num_files);
		num_threads = s.num_files;
	}
	fprintf(stderr, "This machine reports %ld online CPU cores\n",
		sysconf(_SC_NPROCESSORS_ONLN));

	s.out = fopen(argv[2], "w");
	if (s.out == NULL)
	{
		perror("fopen output");
		return (EXIT_FAILURE);
	}
	fprintf(s.out, "Keyword search results for: \"%s\"\n\n", s.keyword);
	pthread_mutex_init(&s.work_lock, NULL);
	pthread_mutex_init(&s.out_lock, NULL);
	threads = malloc(sizeof(pthread_t) * num_threads);
	if (threads == NULL)
	{
		fclose(s.out);
		return (EXIT_FAILURE);
	}
	clock_gettime(CLOCK_MONOTONIC, &start);
	for (i = 0; i < num_threads; i++)
		pthread_create(&threads[i], NULL, worker, &s);
	for (i = 0; i < num_threads; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &end);
	elapsed = (end.tv_sec - start.tv_sec) +
		  (end.tv_nsec - start.tv_nsec) / 1e9;
	fprintf(s.out, "\nTotal occurrences across %d files: %ld\n",
		s.num_files, s.total);
	fclose(s.out);
	pthread_mutex_destroy(&s.work_lock);
	pthread_mutex_destroy(&s.out_lock);
	free(threads);
	printf("Searched %d files for \"%s\" using %d threads "
	       "in %.4f seconds\n", s.num_files, s.keyword,
	       num_threads, elapsed);
	return (EXIT_SUCCESS);
}
