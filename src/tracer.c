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
    memset(buffer, 0, sizeof(buffer)); // Set the buffer to 0
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

    // Check if monitor FIFO exists, else create it
    if (access(MONITOR_FIFO_PATH, F_OK) == -1)
    {
        if (mkfifo(MONITOR_FIFO_PATH, 0666) == -1)
        {
            perror("Failed to create FIFO");
            exit(1);
        }
    }

    // Open the monitor FIFO for reading
    int monitor_fd = open(MONITOR_FIFO_PATH, O_RDONLY);
    if (monitor_fd == -1)
    {
        perror("Failed to open FIFO for reading");
        exit(1);
    }

    // Read and print the entries from the monitor FIFO
    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
    ssize_t num_bytes;
    while ((num_bytes = read(monitor_fd, buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, num_bytes);
    }
    printf("status command finished\n");
    // Close the monitor FIFO
    close(monitor_fd);
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
            char *program_args[] = {program_name, NULL};

            execute_program(program_name, program_args);
        }
        else if (strcmp(option, "-s") == 0)
        {
            query_running_programs();
        }
        else
        {
            printf("Correct Usage: %s <option> [<args>...]\n", argv[0]);
        }
    }

    // TODO: Create a while(1) loop where the user can enter commands
    // The commands are:
    // 1. -u <program> [<args>...] - execute a program
    // 2. -s - request the status of programs from server
    // 3. -q - quit the program
    // 4. anything else is ignored and the user is prompted again

    // TODO FIX THE TRACER GETTING STUCK UPON EXECUTING STATUS COMMAND????????????
    // TODO FIX THE WHILE LOOP ONLY READING PROGRAM NAME AND NOT THE ARGUMENTS
    // TODO POSSIBLY IMPLEMENT PIPELINE EXECUTION OF PROGRAMS (at least on startup)
    while (1)
    {
        printf("\nEnter a new command:\n");
        char command[100];
        fgets(command, sizeof(command), stdin);

        // Remove trailing newline character from the command
        command[strcspn(command, "\n")] = '\0';

        // Tokenize the command to get the option and arguments
        char *option = strtok(command, " ");
        char *program_name = NULL;

        if (option != NULL)
        {
            if (strcmp(option, "-u") == 0)
            {
                program_name = strtok(NULL, " ");
                if (program_name == NULL)
                {
                    printf("Program name is missing\n");
                    continue; // Go to the next iteration of the loop
                }

                char *program_args[] = {program_name, NULL};

                pid_t pid = fork();
                if (pid == -1)
                {
                    perror("Failed to fork");
                    exit(1);
                }
                else if (pid == 0)
                {
                    // Child process
                    execute_program(program_name, program_args);
                    exit(0);
                }
                else
                {
                    // Parent process
                    int status;
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                    {
                        printf("Program execution successful\n");
                    }
                    else
                    {
                        printf("Program execution failed\n");
                    }
                }
            }
            else if (strcmp(option, "-s") == 0)
            {
                query_running_programs();
            }
            else if (strcmp(option, "-q") == 0)
            {
                break; // Exit the loop and end the program
            }
            else
            {
                // Invalid command, prompt again
                continue; // Go to the next iteration of the loop
            }
        }
    }

    return 0;
}
