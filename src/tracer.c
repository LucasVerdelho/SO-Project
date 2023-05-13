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

    // Notify the User about the program execution
    int pid = getpid();
    char notify_buffer[128];
    sprintf(notify_buffer, "Executing %s with PID %d\n", program_name, pid);
    write(STDOUT_FILENO, notify_buffer, strlen(notify_buffer));

    // Get the current timestamp for program start
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Notify the server that a program is about to be executed
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer)); // Set the buffer to 0
    sprintf(buffer, "%d;%s;%ld.%06ld", pid, program_name, start_time.tv_sec, start_time.tv_usec);
    write(fd, buffer, strlen(buffer));

    if (fork() == 0)
    {
        // Execute the program and check for errors
        if (execvp(program_name, program_args) == -1)
        {
            perror("Failed to execute program");

            // Get the current timestamp for program finish
            struct timeval finish_time;
            gettimeofday(&finish_time, NULL);

            // Notify the server that the program has finished with an error
            memset(buffer, 0, sizeof(buffer)); // Clear the buffer
            sprintf(buffer, "%d;finished;%ld.%06ld", pid, finish_time.tv_sec, finish_time.tv_usec);
            write(fd, buffer, strlen(buffer));
            exit(1);
        }
    }
    else
    {
        wait(NULL);

        // Get the current timestamp for program finish
        struct timeval finish_time;
        gettimeofday(&finish_time, NULL);

        // Write to the FIFO again to notify the server that the program has terminated
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer
        sprintf(buffer, "%d;finished;%ld.%06ld", pid, finish_time.tv_sec, finish_time.tv_usec);
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
    // Close the monitor FIFO
    close(monitor_fd);
}

// TODO POSSIBLY IMPLEMENT PIPELINE EXECUTION OF PROGRAMS (at least on startup)

// The main function will take in arguments from the command line
// The commands are:
// 1. -u <program> [<args>...] - execute a program
// 2. -s - request the status of programs from server
// 3. -q - quit the program
// 4. anything else is ignored and the user is prompted again
int main(int argc, char **argv)
{

    // Check number of arguments passed to determine the option selected
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
        int num_args = argc - 3;
        char *program_args[num_args + 2];

        program_args[0] = program_name;
        for (int i = 0; i < num_args; i++)
        {
            program_args[i + 1] = argv[i + 3];
        }
        program_args[num_args + 1] = NULL;

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("Failed to fork");
            exit(1);
        }
        else if (pid == 0)
        {

            execute_program(program_name, program_args);
            exit(0);
        }
    }
    else if (strcmp(option, "-s") == 0)
    {
        query_running_programs();
    }
    else if (strcmp(option, "-q") == 0)
    {
        return 0;
    }
    else
    {
        printf("Invalid option: %s\n", option);
        exit(1);
    }

    // Main loop to read commands from the user and execute them
    while (1)
    {
        // Sleep for 100 milliseconds
        usleep(100000);

        printf("\nEnter a new command:\n");
        char command[100];
        fgets(command, sizeof(command), stdin);

        // Remove trailing newline character from the command
        command[strcspn(command, "\n")] = '\0';

        // Tokenize the command to get the option and arguments
        char *option = strtok(command, " ");
        char *program_name = NULL;
        char *program_args[10]; // Assuming a maximum of 10 program arguments

        if (option != NULL)
        {
            if (strcmp(option, "-u") == 0)
            {
                program_name = strtok(NULL, " ");
                if (program_name == NULL)
                {
                    printf("Program name is missing\n");
                    continue;
                }

                program_args[0] = program_name;
                int arg_index = 1;
                char *arg = strtok(NULL, " ");
                while (arg != NULL)
                {
                    program_args[arg_index++] = arg;
                    arg = strtok(NULL, " ");
                }
                program_args[arg_index] = NULL;

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
                    continue; // Go to the next iteration of the loop
                }
            }
            else if (strcmp(option, "-s") == 0)
            {
                query_running_programs();
            }
            else if (strcmp(option, "-q") == 0)
            {
                // Exit the loop and end the program
                break;
            }
            else
            {
                // Invalid command, prompt again
                continue;
            }
        }
    }

    return 0;
}
