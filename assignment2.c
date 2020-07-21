/*
Author: Lucas Borry
Date: July 21, 2020

CS 344 Oregon State University
*/

/*TODO
-Fix when wc > junk gets stuck
-Error with message displayed after sleep 2 & command
-Fix too many shells running, have to exit multiple times
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

void initializeSignalHandlers();
void reentrantWriteInt(int value);
void handle_SIGCHLD(int sig);
void handle_SIGINT(int sig);
void handle_SIGTSTP(int sig);

int displayExitedProcess = 0;

int main()
{
    // read a line
    char line[2048];
    int status = 0;
    initializeSignalHandlers();

    while (1)
    {
        printf(": ");
        fgets(line, sizeof(line), stdin); //get input from user
        line[strlen(line) - 1] = 0;       //remove \n from string

        char **listOfCommands = parseCommandLine(line); //put user's input into an array of char**

        if (*listOfCommands == NULL)
        {
            //do nothing
        }
        else if (strcmp(listOfCommands[0], "exit") == 0)
        {
            return 0; //exit program if user enters exit
        }
        else if (strcmp(listOfCommands[0], "cd") == 0) //if just cd is entered, go to home directory
        {
            char *directory = NULL;
            if (listOfCommands[1] == NULL)
            {
                directory = getenv("HOME"); //FROM CLASS MODULES, redurect to home directory
            }
            else
            {
                directory = listOfCommands[1];
            }
            int checkDir = chdir(directory);
            if (checkDir != 0)
            {
                printf("%s: no such file or directory\n", directory); //error message if the directory does not exist
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
            int execStatus = executeCommand(listOfCommands, &status); //get exec status to see if the command can be executed
            if (execStatus != 0)
            {
                printf("%s: no such file or directory.\n", listOfCommands[0]); //error message
            }
        }

        fflush(stdout); //clean console
    }
    return 0;
}

/*
This function parses each entry given by the user
ex:
char* parsed = parseCommandLine("hello world");
parsed[0]=="hello"
parsed[1]=="world"
parsed[2]==NULL
*/
char **parseCommandLine(char *commandLine)
{
    int capacity = 2;
    char **array = (char **)calloc(capacity, sizeof(char *)); //initialize the array of size 2
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
            if (ithElement == capacity - 1) //if the array is full
            {
                int newCapacity = capacity * 2;                                 //double capacity size
                char **newArray = (char **)calloc(newCapacity, sizeof(char *)); //allocate new array
                memcpy(newArray, array, sizeof(char *) * capacity);             //copy old array of pointers in new array
                capacity = newCapacity;                                         //reinitialize capacity to new capacity
                array = newArray;                                               //copy elements over
            }
        }
    }
    return array;
}

//Function to execute all the commands given by the user, takes array of commands and the status
int executeCommand(char **argv, int *status)
{
    int i = 0;                 //keep track of where we are in the array
    int backgroundProcess = 0; //used to see if we are in the background or foreground

    char *outFileName = NULL; // file to output in
    char *inFileName = NULL;  //file to input in

    char **commandArray = (char **)calloc(getArraySize(argv), sizeof(char *)); //allocate an array of commands

    while (argv[i] != NULL && strcmp(argv[i], ">") != 0 && strcmp(argv[i], "<") != 0 && strcmp(argv[i], "&") != 0)
    {
        if (strcmp(argv[i], "$$") == 0) //if we are given $$ in the command line
        {
            char processPid[100];
            sprintf(processPid, "%d", getpid()); //print out the pid to the screen
            commandArray[i] = processPid;        //put pid inside command array to replace $$ by pid
        }
        else
        {
            commandArray[i] = argv[i]; //else, copy over elements to command array
        }

        i++; //increase loop counter
    }

    while (argv[i] != NULL)
    {
        if (strcmp(argv[i], "<") == 0) //check if we have a file to input from
        {
            inFileName = argv[i + 1];
            i += 2; //move 2 elements to check if we have a second input or output
        }
        else if (strcmp(argv[i], ">") == 0) //check if we have a file to output in
        {
            outFileName = argv[i + 1];
            i += 2;
        }
        else if (strcmp(argv[i], "&") == 0) //check if the argument & has been passed in the commands
        {
            backgroundProcess = 1;    //if it has, then it is a backgrounf process
            displayExitedProcess = 1; //same thing but global variable used later
            i++;                      //move to next element
        }
    }

    pid_t spawnpid = fork();
    int childStatus;

    if (spawnpid == -1)
    {
        perror("fork() failed!");
        exit(1);
    }

    else if (spawnpid == 0) //child process
    {
        if (outFileName != NULL)
        {
            redirectOutput(outFileName); //redirection of output to new file
        }
        if (inFileName != NULL)
        {
            redirectInput(inFileName); //redirection of input to existing file
        }

        //pass command
        execvp(commandArray[0], commandArray);
        int checkExecStatus = 0; //used to see what status we are at to return it after execvp has been executed
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
            //something go here??
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
    while (argv[i] != NULL) //while loop to count elements
    {
        i++;
    }
    return i;
}

void initializeSignalHandlers()
{
    /***CODE FROM CLASS MODULES***/
    // handle SIGINT
    // Fill out the SIGINT_action struct
    struct sigaction SIGINT_action = {0};
    // Register handle_SIGINT as the signal handler
    SIGINT_action.sa_handler = handle_SIGINT;
    // Block all catchable signals while handle_SIGINT is running
    sigfillset(&SIGINT_action.sa_mask);
    // No flags set
    SIGINT_action.sa_flags = 0;
    // Install our signal handler
    sigaction(SIGINT, &SIGINT_action, NULL);

    // handle SIGTSTP
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // handle SIGCHLD
    struct sigaction SIGCHLD_action = {0};
    SIGCHLD_action.sa_handler = handle_SIGCHLD;
    sigfillset(&SIGCHLD_action.sa_mask);
    SIGCHLD_action.sa_flags = 0;
    sigaction(SIGCHLD, &SIGCHLD_action, NULL);
    /***END OF CODE FROM CLASS MODULES***/
}

//used when we need to put an int into a string with the write() function
void reentrantWriteInt(int value)
{
    if (value == 0) //handle 0 values
        write(0, "0", 1);

    if (value < 0) //handle negative values
    {
        write(0, "-", 1);
        value = -value;
    }

    int max = 1000000000;
    while (max > value)
    {
        max = max / 10;
    }

    //separate each integer individually to be able to print them with the write() function
    while (max > 0)
    {
        int digit = (value / max) % 10;
        char c = (char)((int)'0' + digit);
        write(0, &c, 1);
        max = max / 10;
    }
}

//CODE FROM CLASS MODULES
void handle_SIGCHLD(int sig)
{
    int exitCode;
    pid_t childpid = waitpid(-1, &exitCode, WNOHANG); // non-blocking
    if (exitCode == 0)
    {
        if (displayExitedProcess == 1)
        {
            //PRINT EX: background pid 4923 is done: exit value 0
            write(0, "\nbackground pid ", 16);
            reentrantWriteInt((int)childpid);
            write(0, " is done: exit value ", 21);
            reentrantWriteInt(exitCode);
            write(0, "\n: ", 2);
            displayExitedProcess = 0;
        }
    }
}
//CODE FROM CLASS MODULES
void handle_SIGINT(int sig)
{
    write(0, "SIGINT\n", 7);
}

//CODE FROM CLASS MODULES
void handle_SIGTSTP(int sig)
{
    write(0, "SIGTSTP\n", 7);
}
