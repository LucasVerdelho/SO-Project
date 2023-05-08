#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#define MAX_PIPE_NAME_LEN 128
#define MAX_CMD_LEN 512
#define MAX_ARGS_LEN 256

typedef struct
{
    pid_t pid;
    char name[MAX_CMD_LEN];
    struct timeval start_time;
} Program;

void execute_program(int server_fd, const char *cmd, const char *args);
void list_running_programs(int server_fd);

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("Usage: %s <pipe_name>\n", argv[0]);
        return 1;
    }

    // Open the named pipe for communication with the server
    const char *pipe_name = "/tmp/server_pipe";
    // snprintf(pipe_name, MAX_PIPE_NAME_LEN, "%s.in", argv[1]);
    int server_fd = open(pipe_name, O_WRONLY);
    if (server_fd == -1)
    {
        perror("Failed to open named pipe");
        return 1;
    }

    // Process the command line arguments
    if (strcmp(argv[1], "-u") == 0)
    {
        if (argc < 3)
        {
            printf("Usage: %s -u <program> [<args>...]\n", argv[0]);
            return 1;
        }
        execute_program(server_fd, argv[2], argc > 3 ? argv[3] : NULL);
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        list_running_programs(server_fd);
    }
    else
    {
        printf("Unknown option: %s\n", argv[1]);
        return 1;
    }

    // Close the named pipe and exit
    close(server_fd);
    return 0;
}

void execute_program(int server_fd, const char *cmd, const char *args)
{
    // Prepare the program information
    Program program;
    program.pid = getpid();
    strncpy(program.name, cmd, MAX_CMD_LEN - 1);
    gettimeofday(&program.start_time, NULL);

    // Send the program information to the server
    write(server_fd, &program, sizeof(Program));

    // Notify the user of the PID of the program
    printf("Program PID: %d\n", program.pid);

    // Execute the program
    if (args)
    {
        execlp(cmd, cmd, args, NULL);
    }
    else
    {
        execlp(cmd, cmd, NULL);
    }
    perror("Failed to execute program");
    exit(1);
}

void list_running_programs(int server_fd)
{
    // Send the list running programs request to the server
    write(server_fd, "list", 5);

    // Read the list of running programs from the server
    char buf[MAX_CMD_LEN];
    ssize_t n;
    while ((n = read(server_fd, buf, MAX_CMD_LEN)) > 0)
    {
        buf[n] = '\0';
        printf("%s\n", buf);
    }
    if (n == -1)
    {
        perror("Failed to read from server");
        exit(1);
    }
}
