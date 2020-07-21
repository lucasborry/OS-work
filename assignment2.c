/*
Author: Lucas Borry
Date: July 21, 2020

CS 344 Oregon State University
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

char **parseCommandLine(char *commandLine);
int executeCommand(char **argv, int *status);
int getArraySize(char **argv);
void redirectOutput(char *outFileName);
void redirectInput(char *inFileName);

int main()
{
    // read a line
    char line[2048];
    int status = 0;

    while (1)
    {
        printf(": ");
        fgets(line, sizeof(line), stdin);
        line[strlen(line) - 1] = 0; //remove \n from string

        char **listOfCommands = parseCommandLine(line);

        if (*listOfCommands == NULL)
        {
            //do nothing
        }
        else if (strcmp(listOfCommands[0], "exit") == 0)
        {
            return 0; //exit program if user enters exit
        }
        else if (strcmp(listOfCommands[0], "cd") == 0)
        {
            char *directory = NULL;
            if (listOfCommands[1] == NULL)
            {
                directory = getenv("HOME");
            }
            else
            {
                directory = listOfCommands[1];
            }
            int checkDir = chdir(directory);
            if (checkDir != 0)
            {
                printf("%s: no such file or directory\n", directory);
            }
        }
        else if (strcmp(listOfCommands[0], "status") == 0)
        {
            printf("%d\n", status);
        }
        //do nothing if it's a comment (#text...) or if the entry is none
        else if (listOfCommands[0][0] == '#')
        {
            //nothing happens
        }
        else
        {
            int execStatus = executeCommand(listOfCommands, &status);
            if (execStatus != 0)
            {
                printf("%s: no such file or directory.\n", listOfCommands[0]);
            }
        }

        fflush(stdout);
    }
    return 0;
}
/*
ex:
char* parsed = parseCommandLine("salut les gars");
parsed[0]=="salut"
parsed[1]=="les"
parsed[2]=="gars"
parsed[3]==NULL
*/

char **parseCommandLine(char *commandLine)
{
    int capacity = 2;
    char **array = (char **)calloc(capacity, sizeof(char *));
    int ithElement = 0;
    char *token;
    token = strtok(commandLine, " ");
    array[0] = token;
    /* FROM CLASS MODULE
    walk through other tokens */
    while (token != NULL)
    {
        ithElement++;
        token = strtok(NULL, " ");
        if (token != NULL)
        {
            array[ithElement] = token;
            if (ithElement == capacity - 1)
            {
                int newCapacity = capacity * 2;
                char **newArray = (char **)calloc(newCapacity, sizeof(char *));
                memcpy(newArray, array, sizeof(char *) * capacity);
                capacity = newCapacity;
                array = newArray;
            }
        }
    }
    return array;
}

int executeCommand(char **argv, int *status)
{
    int i = 0;

    int backgroundProcess = 0;

    char *outFileName = NULL;
    char *inFileName = NULL;

    char **commandArray = (char **)calloc(getArraySize(argv), sizeof(char *));

    while (argv[i] != NULL && strcmp(argv[i], ">") != 0 && strcmp(argv[i], "<") != 0 && strcmp(argv[i], "&") != 0)
    {
        if (strcmp(argv[i], "$$") == 0)
        {
            char processPid[100];
            sprintf(processPid, "%d", getpid());
            commandArray[i] = processPid;
        }
        else
        {
            commandArray[i] = argv[i];
        }

        i++;
    }

    while (argv[i] != NULL)
    {
        if (strcmp(argv[i], "<") == 0)
        {
            inFileName = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], ">") == 0)
        {
            outFileName = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], "&") == 0)
        {
            backgroundProcess = 1;
            i++;
        }
    }

    pid_t spawnpid = fork();
    int childStatus;

    if (spawnpid == -1)
    {
        perror("fork() failed!");
        exit(1);
    }

    else if (spawnpid == 0)
    {

        if (outFileName != NULL)
        {
            redirectOutput(outFileName);
        }
        if (inFileName != NULL)
        {
            redirectInput(inFileName);
        }

        //pass command
        execvp(commandArray[0], commandArray);
        int checkExecStatus = 0;
        checkExecStatus = execvp(commandArray[0], commandArray);
        return checkExecStatus;
    }
    else
    {
        //parent process
        if (backgroundProcess == 0)
        {
            spawnpid = waitpid(spawnpid, &childStatus, 0);
            // printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnpid);
        }
        else
        {
        }

        *status = childStatus;
    }
    return 0;
}

//FUNCTION USES CODE FROM MODULES
void redirectOutput(char *outFileName)
{
    int targetFD = open(outFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (targetFD == -1)
    {
        printf("cannot open %s for output\n", outFileName);
        exit(1);
    }
    // Use dup2 to point FD 1, i.e., standard output to targetFD
    int result = dup2(targetFD, 1);
    if (result == -1)
    {
        exit(2);
    }
}

//FUNCTION USES CODE FROM MODULES
void redirectInput(char *inFileName)
{
    // Open source file
    int sourceFD = open(inFileName, O_RDONLY);
    if (sourceFD == -1)
    {
        printf("cannot open %s for input\n", inFileName);

        exit(1);
    }
    int result = dup2(sourceFD, 0);
    if (result == -1)
    {
        exit(2);
    }
}

//function to return the size of an array as an int
int getArraySize(char **argv)
{
    int i = 0;
    while (argv[i] != NULL)
    {
        i++;
    }
    return i;
}