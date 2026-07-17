/*
 * pipeline.c
 * Recreates the shell pipeline "ps aux | grep root" using fork(),
 * execvp() and pipe(), with the parent capturing the pipeline's
 * output into a file and displaying the first lines of it.
 *
 * Process layout:
 *
 *   child 1 (ps aux) --pipe1--> child 2 (grep root) --pipe2--> parent
 *
 * The parent reads everything arriving on pipe2, writes it to
 * pipeline_output.txt, waits for both children, and then prints
 * the first few captured lines to the terminal.
 *
 * Compile: gcc -Wall -Wextra -o pipeline pipeline.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUF_SIZE 4096
#define LINES_TO_SHOW 5

int main(void)
{
	int pipe1[2]; /* ps  -> grep  */
	int pipe2[2]; /* grep -> parent */
	pid_t pid1, pid2;
	int out_fd, status;
	ssize_t bytes_read;
	char buffer[BUF_SIZE];
	FILE *captured;
	char line[BUF_SIZE];
	int shown;

	if (pipe(pipe1) < 0 || pipe(pipe2) < 0)
	{
		perror("pipe");
		return (EXIT_FAILURE);
	}

	/* ---- Child 1: ps aux, stdout into pipe1 ---- */
	pid1 = fork();
	if (pid1 < 0)
	{
		perror("fork child 1");
		return (EXIT_FAILURE);
	}
	if (pid1 == 0)
	{
		char *ps_args[] = {"ps", "aux", NULL};

		dup2(pipe1[1], STDOUT_FILENO);
		close(pipe1[0]);
		close(pipe1[1]);
		close(pipe2[0]);
		close(pipe2[1]);
		execvp(ps_args[0], ps_args);
		perror("execvp ps");
		_exit(EXIT_FAILURE);
	}

	/* ---- Child 2: grep root, stdin from pipe1, stdout into pipe2 ---- */
	pid2 = fork();
	if (pid2 < 0)
	{
		perror("fork child 2");
		return (EXIT_FAILURE);
	}
	if (pid2 == 0)
	{
		char *grep_args[] = {"grep", "root", NULL};

		dup2(pipe1[0], STDIN_FILENO);
		dup2(pipe2[1], STDOUT_FILENO);
		close(pipe1[0]);
		close(pipe1[1]);
		close(pipe2[0]);
		close(pipe2[1]);
		execvp(grep_args[0], grep_args);
		perror("execvp grep");
		_exit(EXIT_FAILURE);
	}

	/* ---- Parent: capture pipe2 into a file ---- */
	close(pipe1[0]);
	close(pipe1[1]);
	close(pipe2[1]);

	out_fd = open("pipeline_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out_fd < 0)
	{
		perror("open output file");
		return (EXIT_FAILURE);
	}

	while ((bytes_read = read(pipe2[0], buffer, BUF_SIZE)) > 0)
	{
		if (write(out_fd, buffer, bytes_read) != bytes_read)
		{
			perror("write output file");
			close(out_fd);
			return (EXIT_FAILURE);
		}
	}

	close(pipe2[0]);
	close(out_fd);

	waitpid(pid1, &status, 0);
	waitpid(pid2, &status, 0);

	/* ---- Display the first captured lines ---- */
	printf("First %d lines of the captured pipeline output:\n\n",
	       LINES_TO_SHOW);
	captured = fopen("pipeline_output.txt", "r");
	if (captured == NULL)
	{
		perror("fopen captured output");
		return (EXIT_FAILURE);
	}
	shown = 0;
	while (shown < LINES_TO_SHOW &&
	       fgets(line, sizeof(line), captured) != NULL)
	{
		printf("%s", line);
		shown++;
	}
	fclose(captured);

	printf("\nFull output captured in pipeline_output.txt\n");
	return (EXIT_SUCCESS);
}
