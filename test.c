#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

pid_t childProcesses[5];

int childProcessesCount()
{
    int count = 0;
    int i;
    for (i = 0; i < 5; i++)
    {
        if (childProcesses[i] != 0)
        {
            count++;
        }
    }
    return count;
}

void removeChildProcess(pid_t processId)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if (childProcesses[i] == processId)
        {
            childProcesses[i] = 0;
        }
    }
}

void addChildProcess(pid_t processId)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if (childProcesses[i] == 0)
        {
            childProcesses[i] = processId;
            break;
        }
    }
}

int main()
{

    memset(childProcesses, 0, 5 * sizeof(pid_t));
    printf("count: %d\n", childProcessesCount());
    addChildProcess(1);
    printf("count: %d\n", childProcessesCount());
    removeChildProcess(1);
    printf("count: %d\n", childProcessesCount());

    return 0;
}