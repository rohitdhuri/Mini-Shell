#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

//limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100

size_t MAX_LINE_LEN = 10000;

// builtin commands
#define EXIT_STR "exit"
#define EXIT_CMD 0
#define UNKNOWN_CMD 99

FILE *fp; // file struct for stdin
char **tokens;
char **tokens1;
char **tokens2;
char *line = "";
int token_count;

void initialize()
{
	// allocate space for the whole line
	assert((line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

	// allocate space for individual tokens
	assert((tokens = malloc(sizeof(char *) * MAX_TOKENS)) != NULL);
	assert((tokens1 = malloc(sizeof(char *) * MAX_TOKENS)) != NULL);
	assert((tokens2 = malloc(sizeof(char *) * MAX_TOKENS)) != NULL);

	// open stdin as a file pointer
	assert((fp = fdopen(STDIN_FILENO, "r")) != NULL);
}

void tokenize(char *string)
{
	int token_count = 0;
	int size = MAX_TOKENS;
	char *this_token;

	while ((this_token = strsep(&string, " \t\v\f\n\r")) != NULL)
	{

		if (*this_token == '\0')
			continue;

		tokens[token_count] = this_token;

		//printf("Token %d: %s\n", token_count, tokens[token_count]);

		token_count++;

		// if there are more tokens than space ,reallocate more space
		if (token_count >= size)
		{
			size *= 2;

			assert((tokens = realloc(tokens, sizeof(char *) * size)) != NULL);
		}
	}
}

void read_command()
{

	// getline will reallocate if input exceeds max length
	assert(getline(&line, &MAX_LINE_LEN, fp) > -1);
	if (line == "")
		return;
	tokenize(line);
}

void split(char **tokens, int c) //splits tokens according to c
{
	int i, j;
	for (i = 0; i < c; i++)
		tokens1[i] = tokens[i];

	tokens1[c] = NULL;
	i = c + 1;
	j = 0;

	while (tokens[i] != NULL)
	{
		tokens2[j] = tokens[i];
		j++;
		i++;
	}
	tokens[i] == NULL;
}

int check_for(char **tokens, char *c) //checks if c is present among tokens
{
	int i = 0;
	//	printf("%s",c);
	while (tokens[i] != NULL)
	{

		if (strcmp(tokens[i], c) == 0)
		{
			return i;
		}
		i++;
	}
	return -1;
}

void basic_cmd(pid_t pid, char **tok) //Executes command without pipes
{
	{
		int fd;
		int i = 0;

		int j = 0;

		while (tok[i] != NULL)
		{
			if (strncmp(tok[i], "<", 1) == 0)
			{
				if (tokens[i + 1] == NULL)
				{
					printf("No arguments \n");
				}

				tok[i] = NULL;
				fd = open(tok[i + 1], O_RDONLY);
				dup2(fd, 0);
				close(fd);
			}
			else if (strncmp(tok[i], ">", 1) == 0)
			{
				if (tok[i + 1] == NULL)
				{
					printf("No arguments \n");
				}

				tok[i] = NULL;
				fd = open(tok[i + 1], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
				dup2(fd, 1);
				close(fd);
			}
			i++;
		}

		// execute cmd
		execvp(tok[0], tok);
		printf("\n%s: command not found", tokens[0]);
		exit(0);
	}
}

int run_command()
{
	int j = 0;
	int pos;
	pid_t pid;
	char *ch = "|";

	if (strcmp(tokens[0], EXIT_STR) == 0)
		return EXIT_CMD;

	//	printf("\nT1 = %s", tokens[0]);

	if (tokens[0] == NULL)
	{
		printf("No input!!");
		return 1;
	}
	pos = check_for(tokens, ch);

	if ((pos >= 0)) //Pipe operations
	{
		int pipefd[2];
		pipe(pipefd);
		split(tokens, pos);

		pid = fork();

		if (pid < 0)
		{
			perror("Fork error \n");
			exit(0);
		}

		if (pid == 0) //child in pipe
		{
			dup2(pipefd[0], 0);
			close(pipefd[1]);
			basic_cmd(pid, tokens2);
			return 1;
		}

		else if (pid > 0) //parent in pipe
		{
			dup2(pipefd[1], 1);
			close(pipefd[0]);
			basic_cmd(pid, tokens1);
			int status = 0;
			waitpid(pid, &status, 0);
			return 1;
		}
	}
	else
	{
		pid = fork();

		if (pid < 0)
		{
			perror("Fork error \n");
			exit(0);
		}

		if (pid == 0) //child process
			basic_cmd(pid, tokens);

		if (pid > 0) //parent process
		{
			int status = 0;
			waitpid(pid, &status, 0);
		}
	}

	return UNKNOWN_CMD;
}

int main()
{
	initialize();

	do
	{
		printf("rohit@sh550> ");
		read_command();
		if (run_command() == EXIT_CMD)
			break;

		memset(tokens, '\0', sizeof(char *) * MAX_TOKENS); //clearing the buffer
		memset(tokens1, '\0', sizeof(char *) * MAX_TOKENS);
		memset(tokens2, '\0', sizeof(char *) * MAX_TOKENS);

	} while (1);

	return 0;
}
