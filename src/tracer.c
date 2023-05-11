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
    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    sprintf(buffer, "%d;%s", pid, program_name);
    write(fd, buffer, strlen(buffer));

    // Notify the user of the program's PID
    printf("Executing program with PID: %d\n", pid);

    if (fork() == 0)
    {
        // Execute the program and check for errors
        if (execvp(program_name, program_args) == -1)
        {
            perror("Failed to execute program");
            memset(buffer, 0, sizeof(buffer)); // Clear the buffer
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
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer
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

    int pid = getpid();

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    sprintf(buffer, "%d;status", pid);
    write(fd, buffer, strlen(buffer));
    close(fd);

    // Open FIFO for reading the response from the server
    fd = open(MONITOR_FIFO_PATH, O_RDONLY);
    if (fd == -1)
    {
        perror("Failed to open FIFO");
        exit(1);
    }

    // Read and print the response from the server
    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
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

    // Invalid number of arguments will print the correct usage and go to loop
    if (argc < 2)
    {
        printf("Correct Usage: %s <option> [<args>...]\n", argv[0]);
    }
    else
    {
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
            printf("No input provided\n");
        }
    }

    // Create a while(1) loop where the user can enter commands
    // The commands are:
    // 1. -u <program> [<args>...] - execute a program
    // 2. -s - request the status of programs from server
    // 3. -q - quit the program
    // 4. anything else is ignored and the user is prompted again
    while (1)
    {
        char buffer[1024];
        memset(buffer, '\0', sizeof(buffer)); // Clear the buffer
        printf("Enter command:\n");
        fgets(buffer, sizeof(buffer), stdin);

        // Split the string into tokens
        char *token = strtok(buffer, " ");
    
        // If the token is NULL then the user entered an empty string
        if (token == NULL)
        {
            continue;
        }

        // If the token is "quit" then exit the program
        if (strcmp(token, "quit") == 0)
        {
            break;
        }

        // If the token is "status" then query the server for the running programs
        if (strcmp(token, "status") == 0)
        {
            query_running_programs();
            continue;
        }

        // If the token is not "quit" or "status" then it must be a program name
        char *program_name = token;

        // Create an array of strings to hold the program arguments
        char *program_args[1024];

        // Add the program name to the array of arguments
        program_args[0] = program_name;

        // Add the rest of the arguments to the array
        int i = 1;
        while ((token = strtok(NULL, " ")) != NULL)
        {
            program_args[i] = token;
            i++;
        }

        // Execute the program
        execute_program(program_name, program_args);
    }
    return 0;
}
