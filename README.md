# Linux Project 2 Assignment

System programming exercises for the Linux Project 2 assignment. Each
question lives in its own folder and contains the source code, the commands
used to build and run it, and any evidence produced while testing. The full
written reports for all four questions are in the accompanying assignment
document.

**Author:** Simileoluwa Oluwatunmise
**Environment:** Ubuntu on WSL2, compiled with gcc using the -Wall and
-Wextra flags. Question 3 and Question 4 also use -pthread.

## Repository structure

| Folder | Question | Summary |
|--------|----------|---------|
| question1 | System Call Tracing and Process Communication | Recreates the shell pipeline that feeds ps aux into grep root using fork, pipe, dup2, and execvp, then traces it with strace |
| question2 | System Calls versus Standard I/O | Two versions of a large file copy utility, one using read and write and one using fread and fwrite, compared with strace |
| question3 | Multithreading and Synchronisation | Counts the prime numbers between 1 and 200,000 using 16 threads and a mutex-protected shared counter |
| question4 | Concurrent File Processing and Synchronisation | Searches multiple text files for a keyword using one thread per file, writing results to a shared output file |

## Question 1: System Call Tracing and Process Communication

**What it does.** pipeline.c recreates the shell pipeline that feeds
ps aux into grep root, but builds it from the process primitives directly
rather than letting the shell do it. It uses fork to create two child
processes, pipe to connect them, dup2 to rewire standard input and output,
and execvp to run ps and grep. The parent captures the final output into
pipeline_output.txt and prints its first five lines.

**Files.**
- pipeline.c, the source
- pipeline_output.txt, the captured output of the pipeline
- trace_full.txt, the complete strace output following all processes
- trace_filtered.txt, the trace reduced to the process, pipe, and file calls

**Build and run.**
```
cd question1
gcc -Wall -Wextra -o pipeline pipeline.c
./pipeline
```

**Trace.**
```
strace -f -o trace_full.txt ./pipeline
```
The -f flag makes strace follow the forked children as well as the parent.
The full trace was then filtered to the relevant calls with grep to produce
trace_filtered.txt.

**Result.** On the test system the pipeline produced 15 root-owned process
lines.

## Question 2: System Calls versus Standard I/O

**What it does.** Two versions of a file copy utility that copy the same
file in 512-byte chunks. copy_syscall.c uses the low-level system calls read
and write, so every chunk is two system calls. copy_stdio.c uses the
standard I/O functions fread and fwrite, which buffer internally and so
reach the kernel far less often. strace counts the system calls each version
makes, which explains the difference in their speed.

**Files.**
- copy_syscall.c and copy_stdio.c, the two versions
- strace_count_syscall.txt and strace_count_stdio.txt, the strace summary
  tables for each version

**Build.**
```
cd question2
gcc -Wall -Wextra -o copy_syscall copy_syscall.c
gcc -Wall -Wextra -o copy_stdio copy_stdio.c
```

**Create the 100 MiB test file.**
```
dd if=/dev/urandom of=largefile.bin bs=1M count=100
```

**Run.**
```
./copy_syscall largefile.bin copy1.bin
./copy_stdio largefile.bin copy2.bin
```

**Count system calls.**
```
strace -c -o strace_count_syscall.txt ./copy_syscall largefile.bin copy1.bin
strace -c -o strace_count_stdio.txt   ./copy_stdio   largefile.bin copy2.bin
```

**Result.** The read and write version made about 409,600 system calls, and
the standard I/O version about 51,200, a difference of eight to one that
comes from the 4,096-byte buffer inside the C library. The standard I/O
version copied the file two to three times faster on the clean runs. The
test file and the two copies are not tracked in the repository and are
regenerated with the commands above.

## Question 3: Multithreading and Synchronisation

**What it does.** prime_counter.c counts the prime numbers between 1 and
200,000 using 16 POSIX threads. The range is split into 16 equal segments of
12,500 numbers, one per thread. Each thread counts the primes in its own
segment into a private counter, then locks a pthread_mutex_t once to add its
subtotal to a single shared counter.

**Files.**
- prime_counter.c, the source

**Build and run.**
```
cd question3
gcc -Wall -Wextra -pthread -o prime_counter prime_counter.c
./prime_counter
```
The -pthread flag is required so the linker finds the thread functions.

**Result.**
```
The synchronized total number of prime numbers between 1 and 200,000 is 17984
```
The total is the same on every run, which shows the mutex is protecting the
shared counter correctly.

## Question 4: Concurrent File Processing and Synchronisation

**What it does.** search.c searches several text files for a keyword. Each
file is read with fopen and fread and searched by a thread, and the result
for every file is written into one shared output file. A mutex guards the
shared list of files so none is processed twice, and a second mutex guards
every write to the output file and the running total. The program also
reports how long the search took, which is used to compare performance at
different thread counts.

**Build.**
```
cd question4
gcc -Wall -Wextra -pthread -o search search.c
```

**Run.**
```
./search keyword output.txt file1.txt file2.txt ... <number_of_threads>
```
The first argument is the keyword, the second is the output file, the
arguments in the middle are the files to search, and the last argument is
the number of threads.

**Recreate the test data.** The sixteen test files are generated rather than
tracked, because together they are about 71 MB. Recreate them with:
```
mkdir data
for i in $(seq 1 16); do yes "the quick brown apple fox jumps over the lazy apple dog" | head -n 80000 > data/file$i.txt; done
```

**Performance test.** The search was timed at three thread counts on a
twelve-core machine: 2 threads, 12 threads for the core count, and 16
threads for the maximum, one per file. The three times were within a few
milliseconds of each other, because this search is limited by memory
bandwidth rather than by processor time, so adding threads beyond the first
few does not make it faster.

**Note.** The per-file lines in output.txt appear in the order the threads
finish, not in file order. This is expected, and the counts and total are
always correct.

## Notes

Compiled programs, the large test file in question2, and the generated data
in question4 are not tracked in the repository. Each of them is rebuilt or
regenerated with the commands above. The written reports for all four
questions are in the accompanying assignment document.
