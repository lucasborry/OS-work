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
void executeCommand(char **argv);
int getArraySize(char **argv);

int main()
{
    // read a line
    char line[1024];

    //test for commands
    char command[] = "a b c d e f";
    char **result = parseCommandLine(command);
    int i = 0;
    while (result[i] != NULL)
    {
        printf("%s\n", result[i]);
        i++;
    }
    //end test

    while (1)
    {
        fgets(line, sizeof(line), stdin);
        line[strlen(line) - 1] = 0; //remove \n from string

        char **listOfCommands = parseCommandLine(line);

        //program if user enters exit
        if (strcmp(listOfCommands[0], "exit") == 0)
        {
            return 0;
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
        }
        //do nothing if it's a comment (#text...)
        else if (listOfCommands[0][0] == '#')
        {
        }
        else
        {
            pid_t spawnpid = fork();
            int childStatus;

            switch (spawnpid)
            {
            case -1:
                perror("fork() failed!");
                exit(1);
                break;
            case 0:
                //child process
                executeCommand(listOfCommands);
                break;
            default:
                //parent process
                spawnpid = waitpid(spawnpid, &childStatus, 0);
                printf("PARENT(%d): child(%d) terminated. Exiting\n", getpid(), spawnpid);
                break;
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

void executeCommand(char **argv)
{
    int i = 0;
    int j = 0;
    int outFound = 0;

    char **commandArray = (char **)calloc(getArraySize(argv), sizeof(char *));
    char **redirectionArray = (char **)calloc(getArraySize(argv), sizeof(char *));

    while (argv[i] != NULL)
    {
        if (strcmp(argv[i], ">") != 0)
        {
            if (outFound == 0)
            {
                commandArray[i] = argv[i];
            }
            else
            {
                redirectionArray[j] = argv[i];
                j++;
            }
        }
        else
        {
            outFound = 1;
        }
        i++;
    }
    if (outFound == 1)
    {
        //CODE FROM MODULES
        int targetFD = open(redirectionArray[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
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
        //END OF CODE FROM MODULES
    }
    //pass command
    execvp(commandArray[0], commandArray);
}

int getArraySize(char **argv)
{
    int i = 0;
    while (argv[i] != NULL)
    {
        i++;
    }
    return i;
}