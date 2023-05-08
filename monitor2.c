#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_NUM_ARGUMENTS 10
#define MAX_NUM_PROCESSES 1024

typedef struct
{
    pid_t pid;
    char command[MAX_COMMAND_LENGTH];
    time_t start_time;
    time_t end_time;
} process_info_t;

process_info_t processes[MAX_NUM_PROCESSES];
int num_processes = 0;

int add_process(pid_t pid, const char *command, time_t start_time)
{
    if (num_processes >= MAX_NUM_PROCESSES)
    {
        return -1;
    }
    processes[num_processes].pid = pid;
    strncpy(processes[num_processes].command, command, MAX_COMMAND_LENGTH - 1);
    processes[num_processes].command[MAX_COMMAND_LENGTH - 1] = '\0';
    processes[num_processes].start_time = start_time;
    num_processes++;
    return 0;
}

void remove_process(pid_t pid, time_t end_time)
{
    int i;
    for (i = 0; i < num_processes; i++)
    {
        if (processes[i].pid == pid)
        {
            printf("Process %d (%s) terminated. Execution time: %ld ms\n",
                   pid, processes[i].command,
                   (end_time - processes[i].start_time) * 1000);
            processes[i].end_time = end_time;
            break;
        }
    }
    if (i < num_processes - 1)
    {
        processes[i] = processes[num_processes - 1];
    }
    num_processes--;
}

void print_processes()
{
    int i;
    for (i = 0; i < num_processes; i++)
    {
        pid_t pid = processes[i].pid;
        const char *command = processes[i].command;
        time_t start_time = processes[i].start_time;
        time_t end_time = processes[i].end_time;
        long execution_time = end_time > start_time ? (end_time - start_time) * 1000 : 0;
        printf("%d %s %ld ms\n", pid, command, execution_time);
    }
}

int main(int argc, char *argv[])
{
    const char *pipe_name = "/tmp/server_pipe";
    int server_fd = -1;

    if (mkfifo(pipe_name, 0666) < 0)
    {
        perror("mkfifo");
        exit(1);
    }

    if ((server_fd = open(pipe_name, O_RDONLY)) < 0)
    {
        perror("open");
        exit(1);
    }

    while (1)
    {
        char buf[MAX_COMMAND_LENGTH];
        int num_read = read(server_fd, buf, MAX_COMMAND_LENGTH - 1);
        if (num_read < 0)
        {
            perror("read");
            break;
        }
        if (num_read == 0)
        {
            // End of file
            break;
        }
        buf[num_read] = '\0';

        char *tokens[MAX_NUM_ARGUMENTS];
        char *saveptr;
        int num_tokens = 0;

        tokens[num_tokens++] = strtok_r(buf, " \t\r\n", &saveptr);
        while (num_tokens < MAX_NUM_ARGUMENTS)
        {
            char *token = strtok_r(NULL, " \t\r\n", &saveptr);
            if (!token)
            {
                break;
            }
            tokens[num_tokens++] = token;
        }

        if (num_tokens == 0)
        {
            // Empty command
            continue;
        }
        else if (strcmp(tokens[0], "exit") == 0)
        {
            // Exit command
            break;
        }
        else if (strcmp(tokens[0], "jobs") == 0)
        {
            // Jobs command
            print_processes();
        }
        else
        {
            // Other command
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("fork");
            }
            else if (pid == 0)
            {
                // Child process
                execvp(tokens[0], tokens);
                perror("execvp");
                exit(1);
            }
            else
            {
                // Parent process
                time_t start_time = time(NULL);
                add_process(pid, buf, start_time);
                int status;
                waitpid(pid, &status, 0);
                time_t end_time = time(NULL);
                remove_process(pid, end_time);
            }
        }
    }

    close(server_fd);
    unlink(pipe_name);
    return 0;
}