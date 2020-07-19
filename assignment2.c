#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>

char **parseCommandLine(char *commandLine);

int main()
{
    // read a line
    char line[1024];
    char command[] = "a b c d e f";
    char **result = parseCommandLine(command);
    int i = 0;
    while (result[i] != NULL)
    {
        printf("%s\n", result[i]);
        i++;
    }

    while (1)
    {
        fgets(line, sizeof(line), stdin);
        line[strlen(line) - 1] = 0; //remove \n from string

        char *token_command;
        const char s[2] = " ";
        token_command = strtok(line, s);

        //program if user enters exit
        if (strcmp(token_command, "exit") == 0)
        {
            return 0;
        }
        else if (strcmp(token_command, "cd") == 0)
        {
            char *token_path = strtok(NULL, s);
            printf("change dir to: %s\n", token_path);
            int checkDir = chdir(token_path);
            if (checkDir != 0)
            {
                printf("%s: no such file or directory\n", token_path);
            }
        }
        else if (strcmp(token_command, "status") == 0)
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
                execlp(token_command, "-al");
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
    char **array = calloc(capacity, sizeof(char *));
    int ithElement = 0;
    char *token;
    token = strtok(commandLine, " ");
    char *newToken = malloc(strlen(token));
    strcpy(newToken, token);
    array[0] = newToken;
    /* walk through other tokens */
    while (token != NULL)
    {
        ithElement++;
        token = strtok(NULL, " ");
        if (token != NULL)
        {
            newToken = malloc(strlen(token));
            strcpy(newToken, token);
            array[ithElement] = newToken;
            if (ithElement == capacity - 1)
            {
                int newCapacity = capacity * 2;
                char **newArray = calloc(newCapacity, sizeof(char *));
                memcpy(newArray, array, sizeof(char *) * capacity);
                capacity = newCapacity;
                array = newArray;
            }
        }
    }
    return array;
}