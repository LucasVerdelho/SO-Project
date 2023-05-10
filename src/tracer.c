#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#define TRACER_FIFO_PATH "../tmp/tracer_fifo"
#define MONITOR_FIFO_PATH "../tmp/monitor_fifo"

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

    int pid = getpid();

    // Notify the server that a program is about to be executed
    char buffer[1024];
    sprintf(buffer, "%d;%s", pid, program_name);
    printf("buffer: %s\n", buffer);
    write(fd, buffer, strlen(buffer));

    // Notify the user of the program's PID
    printf("Executing program with PID: %d\n", pid);

    if (fork() == 0)
    {
        // Execute the program and check for errors
        if (execvp(program_name, program_args) == -1)
        {
            perror("Failed to execute program");
            sprintf(buffer, "%d;finished", pid);
            write(fd, buffer, strlen(buffer));
            exit(1);
        }
    }
    else
    {
        wait(NULL);

        // Write to the FIFO again to notify the server that the program has terminated
        printf("Program %d terminated\n", pid);
        sprintf(buffer, "%d;finished", pid);
        write(fd, buffer, strlen(buffer));
    }

    // Close FIFO
    close(fd);
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
