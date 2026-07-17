/*
 * prime_counter.c
 * Counts the prime numbers between 1 and 200,000 using 16 POSIX
 * threads. The range is split into 16 equal segments, one per
 * thread. Each thread counts the primes in its own segment into a
 * private local counter, then locks a shared mutex once to add that
 * subtotal to the global counter. The mutex guarantees the shared
 * counter is updated by only one thread at a time.
 *
 * Compile: gcc -Wall -Wextra -pthread -o prime_counter prime_counter.c
 * Run:     ./prime_counter
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define RANGE_START 1
#define RANGE_END   200000
#define NUM_THREADS 16

/* Shared counter and the mutex that protects it */
static long prime_total;
static pthread_mutex_t total_lock;

/* The slice of the range handed to one thread */
typedef struct segment_s
{
	int start;
	int end;
} segment_t;

/*
 * is_prime - test whether n is a prime number
 * Returns 1 if n is prime, 0 otherwise. Only odd divisors up to the
 * square root of n are tested, which is enough to decide primality.
 */
static int is_prime(int n)
{
	int i;

	if (n < 2)
		return (0);
	if (n == 2)
		return (1);
	if (n % 2 == 0)
		return (0);
	for (i = 3; i * i <= n; i += 2)
	{
		if (n % i == 0)
			return (0);
	}
	return (1);
}

/*
 * count_segment - thread routine: count primes in one segment
 * Counts into a private local variable first, then takes the mutex
 * once to add the subtotal to the shared counter.
 */
static void *count_segment(void *arg)
{
	segment_t *seg = (segment_t *)arg;
	long local = 0;
	int n;

	for (n = seg->start; n <= seg->end; n++)
	{
		if (is_prime(n))
			local++;
	}
	pthread_mutex_lock(&total_lock);
	prime_total += local;
	pthread_mutex_unlock(&total_lock);
	return (NULL);
}

int main(void)
{
	pthread_t threads[NUM_THREADS];
	segment_t segments[NUM_THREADS];
	int i;
	int total_numbers = RANGE_END - RANGE_START + 1;
	int base = total_numbers / NUM_THREADS;
	int remainder = total_numbers % NUM_THREADS;
	int next = RANGE_START;

	if (pthread_mutex_init(&total_lock, NULL) != 0)
	{
		fprintf(stderr, "Failed to initialise mutex\n");
		return (EXIT_FAILURE);
	}
	for (i = 0; i < NUM_THREADS; i++)
	{
		int count = base + (i < remainder ? 1 : 0);

		segments[i].start = next;
		segments[i].end = next + count - 1;
		next += count;
		if (pthread_create(&threads[i], NULL,
				   count_segment, &segments[i]) != 0)
		{
			fprintf(stderr, "Failed to create thread %d\n", i);
			return (EXIT_FAILURE);
		}
	}
	for (i = 0; i < NUM_THREADS; i++)
		pthread_join(threads[i], NULL);
	pthread_mutex_destroy(&total_lock);
	printf("The synchronized total number of prime numbers "
	       "between 1 and 200,000 is %ld\n", prime_total);
	return (EXIT_SUCCESS);
}
