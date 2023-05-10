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

typedef struct ExecutionInfo
{
    pid_t pid;
    char program_name[256];
    struct timeval start_time;
    struct timeval end_time;
    struct ExecutionInfo *next;
} ExecutionInfo;

ExecutionInfo *execution_list = NULL; // Linked list to store ExecutionInfo

void handle_query_request()
{
    // Prepare the response based on the execution information in the linked list
    char response[1024];
    response[0] = '\0';

    ExecutionInfo *curr = execution_list;
    while (curr != NULL)
    {
        // Format the response based on the execution information
        char entry[256];
        snprintf(entry, sizeof(entry), "PID: %d, Program: %s\n", curr->pid, curr->program_name);
        strncat(response, entry, sizeof(response) - strlen(response) - 1);

        curr = curr->next;
    }

    // Open FIFO for writing
    int fd = open(MONITOR_FIFO_PATH, O_WRONLY);
    if (fd == -1)
    {
        perror("Failed to open FIFO");
        exit(1);
    }

    // Send the response to the client
    write(fd, response, strlen(response));

    // Close FIFO
    close(fd);
}

int main()
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

    // Open FIFO for reading
    int tracer_fd;
    if (tracer_fd = open(TRACER_FIFO_PATH, O_RDWR) == -1)
    {
        perror("Failed to open FIFO");
        exit(1);
    }

    while (1)
    {

        // Read the client's request
        // The structure of a client request is as follows:
        // proccess ID;program name/status(finish signal)
        char buffer[1024];
        ssize_t num_bytes = read(tracer_fd, buffer, sizeof(buffer));
        if (num_bytes == -1)
        {
            perror("Failed to read FIFO");
            exit(1);
        }

        // Handle the client's request
        if (strcmp(buffer, "query") == 0)
        {
            handle_query_request();
        }
        else
        {
            ExecutionInfo *exec_info = (ExecutionInfo *)buffer;
            // handle_execution_info(exec_info);
        }
    }

    // Close FIFO
    close(tracer_fd);

    return 0;
}
