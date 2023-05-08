#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_ARGS 10

int main(int argc, char *argv[])
{
    int pipe_fd;
    char *pipe_name = "my_pipe";
    char *args[MAX_ARGS];
    char buf[256];
    int i;
    pid_t pid;
    struct timeval start_time, end_time;
    long int elapsed_time;

    if (argc < 2)
    {
        printf("Usage: %s [-u] program_name [arguments...]\n", argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "-u") != 0)
    {
        printf("Error: Invalid option %s\n", argv[1]);
        exit(1);
    }

    for (i = 2; i < argc && i - 2 < MAX_ARGS; i++)
    {
        args[i - 2] = argv[i];
    }
    args[i - 2] = NULL;

    gettimeofday(&start_time, NULL);

    pid = fork();
    if (pid == -1)
    {
        printf("Error: Fork failed\n");
        exit(1);
    }
    else if (pid == 0)
    {
        execvp(argv[2], args);
        printf("Error: Failed to execute program %s\n", argv[2]);
        exit(1);
    }
    else
    {
        printf("PID of program %s is %d\n", argv[2], pid);

        gettimeofday(&end_time, NULL);
        elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;

        pipe_fd = open(pipe_name, O_WRONLY);
        if (pipe_fd == -1)
        {
            printf("Error: Failed to open pipe %s\n", pipe_name);
            exit(1);
        }

        sprintf(buf, "execute %d %s %ld\n", pid, argv[2], elapsed_time);
        if (write(pipe_fd, buf, strlen(buf)) == -1)
        {
            printf("Error: Failed to write to pipe\n");
            exit(1);
        }

        close(pipe_fd);
    }

    return 0;
}
