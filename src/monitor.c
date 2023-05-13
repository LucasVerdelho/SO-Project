#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>

#define TRACER_FIFO_PATH "../tmp/tracer_fifo"
#define MONITOR_FIFO_PATH "../tmp/monitor_fifo"
#define MAX_ENTRIES 256

typedef struct
{
    int pid;
    char command[256];
    struct timeval start_time;
    struct timeval end_time;
} ExecutionInfo;

ExecutionInfo executionInfos[MAX_ENTRIES];
int numEntries = 0;

void insert_entry(int pid, const char *command)
{
    if (numEntries >= MAX_ENTRIES)
    {
        printf("Maximum number of entries reached.\n");
        return;
    }

    executionInfos[numEntries].pid = pid;
    strncpy(executionInfos[numEntries].command, command, sizeof(executionInfos[numEntries].command));
    gettimeofday(&executionInfos[numEntries].start_time, NULL);
    numEntries++;
}

void update_entry(int pid, int status)
{
    for (int i = 0; i < numEntries; i++)
    {
        if (executionInfos[i].pid == pid)
        {
            if (status == 0)
            {
                printf("Process with PID %d exited with status 0\n", pid);
                gettimeofday(&executionInfos[i].end_time, NULL);
            }
            else
            {
                printf("Process with PID %d exited with non-zero status\n", pid);
            }
            return;
        }
    }

    printf("Entry not found for PID: %d\n", pid);
}

void print_entries()
{

    // Check if monitor Fifo exists, else create it
    if (access(MONITOR_FIFO_PATH, F_OK) == -1)
    {
        if (mkfifo(MONITOR_FIFO_PATH, 0666) == -1)
        {
            perror("Failed to create FIFO");
            exit(1);
        }
    }

    // Open the monitor FIFO for writing
    int monitor_fd = open(MONITOR_FIFO_PATH, O_WRONLY);
    if (monitor_fd == -1)
    {
        perror("Failed to open FIFO");
        exit(1);
    }

    write(monitor_fd, "PID;PROGRAM;DURATION(ms)\n", 26);
    char buffer[1024];
    long start_ms, end_ms, duration_ms;

    for (int i = 0; i < numEntries; i++)
    {

        // In case the process is still running, calculate the duration until now
        if (executionInfos[i].end_time.tv_sec == 0 && executionInfos[i].end_time.tv_usec == 0)
        {
            printf("Process with PID %d is still running\n", executionInfos[i].pid);
            start_ms = (executionInfos[i].start_time.tv_sec * 1000) + (executionInfos[i].start_time.tv_usec / 1000);
            // Calculate ms from start time until now
            struct timeval now;
            gettimeofday(&now, NULL);
            end_ms = (now.tv_sec * 1000) + (now.tv_usec / 1000);
            duration_ms = end_ms - start_ms;
        }
        else // Process has finished
        {
            start_ms = (executionInfos[i].start_time.tv_sec * 1000) + (executionInfos[i].start_time.tv_usec / 1000);
            end_ms = (executionInfos[i].end_time.tv_sec * 1000) + (executionInfos[i].end_time.tv_usec / 1000);
            duration_ms = end_ms - start_ms;
        }

        // Notify the client program of each program's execution info
        sprintf(buffer, "%d;%s;%ld\n", executionInfos[i].pid, executionInfos[i].command, duration_ms);
        write(monitor_fd, buffer, strlen(buffer));
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer

        printf("PID: %d, Command: %s\n", executionInfos[i].pid, executionInfos[i].command);
        printf("Duration: %ld milliseconds\n\n", duration_ms);
    }
    close(monitor_fd);
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
    if ((tracer_fd = open(TRACER_FIFO_PATH, O_RDWR)) == -1)
    {
        perror("Failed to open FIFO");
        exit(1);
    }

    while (1)
    {
        printf("\nWaiting for client update...\n");

        // Read the client's request
        // The structure of a client request is as follows:
        // process ID;program name/status(finish signal)
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        ssize_t num_bytes = read(tracer_fd, buffer, sizeof(buffer));
        if (num_bytes == -1)
        {
            perror("Failed to read FIFO");
            exit(1);
        }

        // Parse the client's request
        int pid;
        char info[256];
        sscanf(buffer, "%d;%s", &pid, info);

        printf("Received Update - PID: %d, Info: %s\n", pid, info);

        if (strcmp(info, "finished") == 0)
        {
            // printf("Process with PID %d finished\n", pid);
            update_entry(pid, 0);
        }
        else if (strcmp(info, "status") == 0)
        {
            print_entries();
        }
        else
        {
            // printf("Process with PID %d started\n", pid);
            insert_entry(pid, info);
        }
        // Reset the buffer
        memset(buffer, 0, sizeof(buffer));
    }

    // Close FIFO
    close(tracer_fd);

    return 0;
}