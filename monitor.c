#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_BUF 1024
#define PIPE_NAME "monitor_pipe"

typedef struct program_info
{
    pid_t pid;
    char name[MAX_BUF];
    struct timeval start_time;
    struct timeval end_time;
} program_info_t;

int main(int argc, char *argv[])
{
    int pipe_fd;
    char buf[MAX_BUF];
    int bytes_read;
    program_info_t programs[MAX_BUF];
    int num_programs = 0;

    // Create named pipe
    mkfifo(PIPE_NAME, 0666);

    // Open named pipe for reading
    pipe_fd = open(PIPE_NAME, O_RDONLY);

    while (1)
    {
        // Read from named pipe
        bytes_read = read(pipe_fd, buf, MAX_BUF);

        if (bytes_read > 0)
        {
            // Null-terminate the buffer
            buf[bytes_read] = '\0';

            // Parse the buffer
            char *token = strtok(buf, " ");
            if (strcmp(token, "EXECUTE") == 0)
            {
                // Execute the program
                pid_t pid = fork();
                if (pid == 0)
                {
                    // Child process
                    char **args = (char **)malloc((argc - 2) * sizeof(char *));
                    for (int i = 0; i < argc - 2; i++)
                    {
                        args[i] = argv[i + 2];
                    }
                    args[argc - 2] = NULL;
                    execvp(argv[2], args);
                    exit(0);
                }
                else
                {
                    // Parent process
                    // Get current timestamp
                    struct timeval current_time;
                    gettimeofday(&current_time, NULL);

                    // Save program information
                    program_info_t program_info;
                    program_info.pid = pid;
                    strncpy(program_info.name, argv[2], MAX_BUF);
                    program_info.start_time = current_time;
                    programs[num_programs++] = program_info;

                    // Send program information to server via named pipe
                    int server_pipe_fd = open(PIPE_NAME, O_WRONLY);
                    char server_buf[MAX_BUF];
                    sprintf(server_buf, "START %d %s %ld %ld\n", pid, program_info.name, program_info.start_time.tv_sec, program_info.start_time.tv_usec);
                    write(server_pipe_fd, server_buf, strlen(server_buf));
                    close(server_pipe_fd);

                    // Print PID to standard output
                    printf("PID of program %s is %d\n", argv[2], pid);
                }
            }
            else if (strcmp(token, "STATUS") == 0)
            {
                // Print program status
                for (int i = 0; i < num_programs; i++)
                {
                    program_info_t program_info = programs[i];

                    // Get current timestamp
                    struct timeval current_time;
                    gettimeofday(&current_time, NULL);

                    // Calculate execution time
                    long int execution_time = (current_time.tv_sec - program_info.start_time.tv_sec) * 1000 + (current_time.tv_usec - program_info.start_time.tv_usec) / 1000;

                    printf("%d %s %ld\n", program_info.pid, program_info.name, execution_time);
                }
            }
            else
            {
                printf("Invalid command\n");
            }
        }
    }

    // Close named pipe
    close(pipe_fd);
    unlink(PIPE_NAME);

    return 0;
}
