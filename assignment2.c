// programme qui lit en boucle (infinie) des entrees de l'utilisateur
// modifier pour qu'il quitte quand l'utilisateur entre "exit"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

char **parseCommandLine(char *commandLine);
void executeCommand(char **argv, int *status);
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
            printf("change dir to: %s\n", listOfCommands[1]);
            int checkDir = chdir(listOfCommands[1]);
            if (checkDir != 0)
            {
                printf("%s: no such file or directory\n", listOfCommands[1]);
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
            executeCommand(listOfCommands, &status);
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

void executeCommand(char **argv, int *status)
{
    pid_t spawnpid = fork();
    int childStatus;

    if (spawnpid == -1)
    {
        perror("fork() failed!");
        exit(1);
    }

    else if (spawnpid == 0)
    {
        //child process
        int i = 0;

        char *outFileName = NULL;
        char *inFileName = NULL;
        int backgroundProcess = 0;

        char **commandArray = (char **)calloc(getArraySize(argv), sizeof(char *));

        while (argv[i] != NULL && strcmp(argv[i], ">") != 0 && strcmp(argv[i], "<") != 0 && strcmp(argv[i], "&") == 0)
        {
            commandArray[i] = argv[i];
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
    }
    else
    {
        //parent process
        spawnpid = waitpid(spawnpid, &childStatus, 0);
        // printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnpid);
        *status = childStatus;
    }
}

//FUNCTION USES CODE FROM MODULES
void redirectOutput(char *outFileName)
{
    int targetFD = open(outFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (targetFD == -1)
    {
        perror("open()");
        exit(1);
    }
    // Use dup2 to point FD 1, i.e., standard output to targetFD
    int result = dup2(targetFD, 1);
    if (result == -1)
    {
        perror("dup2");
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