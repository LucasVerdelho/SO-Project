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


void handle_execution_info(ExecutionInfo *exec_info)
{
    // Calculate execution time
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    long execution_time = (end_time.tv_sec - exec_info->start_time.tv_sec) * 1000 +
                          (end_time.tv_usec - exec_info->start_time.tv_usec) / 1000;

    // Print execution time
    printf("Execution time for PID %d: %ld milliseconds\n", exec_info->pid, execution_time);

    // Store execution information in a file or data structure
    // ...

    // You can perform any additional processing required based on the execution info received
}

void handle_query_request()
{

    // Check if fifo on MONITOR_FIFO_PATH exists and if not create it
    if (access(MONITOR_FIFO_PATH, F_OK) == -1)
    {
        if (mkfifo(MONITOR_FIFO_PATH, 0666) == -1)
        {
            perror("Failed to create FIFO");
            exit(1);
        }
    }

    // Read the execution information from the file or data structure
    // ...

    // Prepare the response
    char response[1024];
    // Format the response based on the execution information
    // ...

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

    while (1)
    {
        // Open FIFO for reading
        int fd = open(TRACER_FIFO_PATH, O_RDONLY);
        if (fd == -1)
        {
            perror("Failed to open FIFO");
            exit(1);
        }

        // Read the client's request
        char buffer[1024];
        ssize_t num_bytes = read(fd, buffer, sizeof(buffer));
        if (num_bytes == -1)
        {
            perror("Failed to read FIFO");
            exit(1);
        }

        // Close FIFO
        close(fd);

        // Handle the client's request
        if (strcmp(buffer, "query") == 0)
        {
            handle_query_request();
        }
        else
        {
            ExecutionInfo *exec_info = (ExecutionInfo *)buffer;
            handle_execution_info(exec_info);
        }
    }

    return 0;
}
