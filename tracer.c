#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#define TRACER_FIFO_PATH "/tmp/tracer_fifo"
#define MONITOR_FIFO_PATH "/tmp/monitor_fifo"

typedef struct
{
    pid_t pid;
    char program_name[256];
    struct timeval start_time;
} ExecutionInfo;

void execute_program(char *program_name, char **program_args)
{
    // Check if fifo on TRACER_FIFO_PATH exists and if not create it
    if (access(TRACER_FIFO_PATH, F_OK) == -1)
    {
        if (mkfifo(TRACER_FIFO_PATH, 0666) == -1)
        {
            perror("Failed to create FIFO");
            exit(1);
        }
    }

    // Open FIFO for writing
    int fd = open(TRACER_FIFO_PATH, O_WRONLY);
    if (fd == -1)
    {
        perror("Failed to open FIFO");
        exit(1);
    }

    // Prepare execution info
    ExecutionInfo exec_info;
    exec_info.pid = getpid();
    strcpy(exec_info.program_name, program_name);
    gettimeofday(&exec_info.start_time, NULL);

    // Send execution info to the server
    write(fd, &exec_info, sizeof(ExecutionInfo));

    // Notify the user of the program's PID
    printf("Executing program with PID: %d\n", exec_info.pid);

    // Close FIFO
    close(fd);

    // Execute the program and check for errors
    if (execvp(program_name, program_args) == -1)
    {
        perror("Failed to execute program");
        exit(1);
    }
}

void query_running_programs()
{
    // Open FIFO for writing
    int fd = open(TRACER_FIFO_PATH, O_WRONLY);
    if (fd == -1)
    {
        perror("Failed to open FIFO");
        exit(1);
    }

    // Send the query request to the server
    write(fd, "query", strlen("query"));

    // Close FIFO
    close(fd);

    // Open FIFO for reading
    fd = open(MONITOR_FIFO_PATH, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open FIFO");
        exit(1);
    }

    // Read and print the response from the server
    char buffer[1024];
    ssize_t num_bytes;
    while ((num_bytes = read(fd, buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, num_bytes);
    }

    // Close FIFO
    close(fd);
}

int main(int argc, char **argv)
{
    // Check number of arguments passed to determine the option selected

    // Invalid number of arguments
    if (argc < 2)
    {
        printf("Correct Usage: %s <option> [<args>...]\n", argv[0]);
        exit(1);
    }

    // Get the option selected
    char *option = argv[1];

    // If the option is "-u" then execute the program(s) 
    if (strcmp(option, "-u") == 0)
    {
        if (argc < 3)
        {
            printf("Program name is missing\n");
            exit(1);
        }

        char *program_name = argv[2];
        char **program_args = &argv[2];

        execute_program(program_name, program_args);
    }
    else if (strcmp(option, "-s") == 0)
    {
        query_running_programs();
    }
    else
    {
        printf("Invalid option\n");
        exit(1);
    }

    return 0;
}
