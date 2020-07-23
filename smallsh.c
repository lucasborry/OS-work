/*
Author: Lucas Borry
Date: July 21, 2020

CS 344 Oregon State University

Program that creates a shell to take commands from the user, similar to bash.
Supports three built in commands: exit, cd, and status.
Supports comments, which are lines beginning with the # character.
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
int executeCommand(char **argv, int backgroundMode);
char *replaceDollarDollar(char *original, char *newValue);
int getArraySize(char **argv);
void redirectOutput(int targetFD);
void redirectInput(int sourceFD);

//signal functions

void enable_SIGINT();
void enable_SIGTSTP();
void enable_SIGTERM();
void handle_SIGINT(int sig);
void handle_SIGTSTP(int sig);
void disable_SIGINT();
void disable_SIGTSTP();
void printLastChildProcessStatus();

void reentrantWriteInt(int value);

int displayExitedProcess = 0;
int lastForegroundProcessPid = 0;
int lastBackgroundProcessPid = 0;
int killSignal = 0;
int status = 0;
int foregroundOnlyMode = 0;

int main()
{
    // allocate memory for an input line
    char line[2048];

    enable_SIGINT();
    enable_SIGTSTP();
    enable_SIGTERM();

    int lastForegroundMode = 0;
    while (1)
    {
        if (lastForegroundMode != foregroundOnlyMode) //check to see if we are in foreground only or not
        {
            if (lastForegroundMode == 0)
            {
                printf("Entering foreground-only mode (& is now ignored)\n");
            }
            else
            {
                printf("Exiting foreground-only mode\n");
            }
            lastForegroundMode = foregroundOnlyMode;
        }

        printLastChildProcessStatus();
        printf(": ");
        fflush(stdout);
        char *chars = fgets(line, sizeof(line), stdin); //get input from user
        if (chars == NULL)
        {
            clearerr(stdin); // reset stdin status
            continue;
        }
        int backgroudMode = 0;
        line[strlen(line) - 1] = 0; //remove \n from string
        if (line[strlen(line) - 1] == '&')
        {
            backgroudMode = 1;
            line[strlen(line) - 1] = 0; //remove '&' from string
        }
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
                fflush(stdout);
            }
        }
        else if (strcmp(listOfCommands[0], "status") == 0)
        {
            if (status != 0)
            {
                printf("%d\n", status);
                fflush(stdout);
            }
            else if (killSignal != 0)
            {
                printf(" terminated by signal %d\n", killSignal);
                fflush(stdout);
            }
            else
            {
                printf("0\n");
                fflush(stdout);
            }
        }
        //do nothing if it's a comment (#text...) or if the entry is none
        else if (listOfCommands[0][0] == '#')
        {
            //nothing happens
        }
        else
        {
            int execStatus = executeCommand(listOfCommands, backgroudMode); //get exec status to see if the command can be executed
        }

        fflush(stdout); //clean console
    }
    return 0;
}

/*
This function parses each entry given by the user
ex:
char* parsed = parseCommandLine("wc > junk");
parsed[0]=="wc"
parsed[1]==">"
parsed[2]=="junk"
parsed[3]==NULL
*/
char **parseCommandLine(char *commandLine)
{
    char processPid[100];
    sprintf(processPid, "%d", getpid()); //formats the pid to a string

    int capacity = 2;
    char **array = (char **)calloc(capacity, sizeof(char *)); //initialize the array of size 2
    int ithElement = 0;
    char *token;
    token = strtok(commandLine, " ");
    array[0] = token;
    /*CODE FROM CLASS MODULES
    walk through other tokens */
    while (token != NULL)
    {
        ithElement++;
        token = strtok(NULL, " ");
        //END OF CODE FROM CLASS MODULES
        if (token != NULL)
        {
            array[ithElement] = replaceDollarDollar(token, processPid);

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
int executeCommand(char **argv, int backgroundMode)
{
    status = 0;
    killSignal = 0;

    int i = 0; //keep track of where we are in the array

    char *outFileName = NULL; // file to output in
    char *inFileName = NULL;  //file to input in

    char **commandArray = (char **)calloc(getArraySize(argv), sizeof(char *)); //allocate an array of commands

    while (argv[i] != NULL && strcmp(argv[i], ">") != 0 && strcmp(argv[i], "<") != 0)
    {

        commandArray[i] = argv[i]; //else, copy over elements to command array
        i++;                       //increase loop counter
    }

    commandArray[i] = '\0'; //insert null char at the end

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
        else
        {
            i++; //move to next element
        }
    }

    pid_t spawnpid = fork(); //fork child process
    int childStatus;         //keep track of child status

    if (spawnpid == -1)
    {
        perror("fork() failed!");
        exit(1);
    }

    else if (spawnpid == 0) //child process
    {
        int canRun = 1; //check to see if input is existing or not, none existing is 0
        int outTarget = -1;
        if (outFileName != NULL)
        {
            outTarget = open(outFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644); //FROM CLASS MODULES
            if (outTarget == -1)
            {
                printf("cannot open %s for output\n", outFileName);
                fflush(stdout);
                canRun = 0;
            }
        }
        else if (backgroundMode == 1)
            outTarget = open("/dev/null", O_WRONLY, 0); //open null directory
        if (outTarget != -1)
            redirectOutput(outTarget); //redirection of output to new file

        int inTarget = -1;
        if (inFileName != NULL)
        {
            inTarget = open(inFileName, O_RDONLY); //FROM CLASS MODULES
            if (inTarget == -1)
            {
                printf("cannot open %s for intput\n", inFileName);
                fflush(stdout);
                canRun = 0;
            }
        }

        else if (backgroundMode == 1)
            inTarget = open("/dev/null", O_RDONLY, 0); //open null directory
        if (inTarget != -1)
            redirectInput(inTarget); //redirection of input to existing file

        //pass command
        int checkExecStatus = 0; //used to see what status we are at to return it after execvp has been executed
        if (backgroundMode == 0)
        {
            lastForegroundProcessPid = getpid(); //give the pid to the global variable
        }
        else
        {
            lastForegroundProcessPid = 0; //otherwise there isn't one
        }
        if (canRun) //if we can run, then we execute the command
        {
            checkExecStatus = execvp(commandArray[0], commandArray);
            if (checkExecStatus != 0)
            {
                printf("%s: no such file or directory.\n", commandArray[0]); //error message
                fflush(stdout);
            }
        }
        exit(1);
    }

    else
    {
        //parent process
        if (backgroundMode == 0 || foregroundOnlyMode != 0) //do not wait for background commands
        {
            spawnpid = waitpid(spawnpid, &childStatus, 0);
        }
        else
        {
            printf("background pid is %d\n", spawnpid);
            fflush(stdout);
            lastBackgroundProcessPid = spawnpid;
        }
        if (childStatus != 0)
            status = 1;
        else
            status = childStatus; //make the global var equal to child status

        killSignal = 0; // we didn't kill
    }
    return 0;
}

//replace $$ by another string (pid)
char *replaceDollarDollar(char *original, char *newValue)
{
    char *result = (char *)malloc(1000); //allocate new memory
    result[0] = '\0';
    int indexOfDol = -1; //keep track of where the dollar sign is
    int i = 0;
    while (original[i] != 0)
    {
        if (original[i] == '$' && original[i + 1] == '$') //go through the array and make the position of $ = indexOfDol
        {
            indexOfDol = i;
            break;
        }
        i++;
    }

    if (indexOfDol != -1)
    {
        strncpy(result, original, indexOfDol); //copy original into result
        strcat(result, newValue);
        return result;
    }
    else
        return original;
}

//FUNCTION USES CODE FROM MODULES
void redirectOutput(int targetFD)
{
    if (targetFD == -1)
    {
        printf("cannot open file for output\n");
        fflush(stdout);
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
void redirectInput(int sourceFD)
{
    // Open source file
    if (sourceFD == -1)
    {
        printf("cannot open file for input\n");
        fflush(stdout);

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

/***CODE FROM CLASS MODULES***/
void enable_SIGINT()
{
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
}

void enable_SIGTSTP()
{
    // handle SIGTSTP
    struct sigaction SIGTSTP_action = {0};
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = 0;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}
/***END OF CODE FROM CLASS MODULES***/
void enable_SIGTERM()
{
    // handle SIGTSTP
    struct sigaction SIGTERM_action = {0};
    SIGTERM_action.sa_handler = handle_SIGINT;
    sigfillset(&SIGTERM_action.sa_mask);
    SIGTERM_action.sa_flags = 0;
    sigaction(SIGTERM, &SIGTERM_action, NULL);
}
void printLastChildProcessStatus()
{
    int exitCode;
    pid_t childpid = waitpid(-1, &exitCode, WNOHANG); // non-blocking, from modules
    if (exitCode == 0 && lastBackgroundProcessPid != 0)
    {
        if (childpid == 0 && killSignal != 0)
        {
            printf("background pid %d is done: terminated by signal %d\n", lastBackgroundProcessPid, killSignal);
            fflush(stdout);
            lastBackgroundProcessPid = 0;
        }
        else if (childpid != 0 && killSignal == 0)
        {
            printf("background pid %d is done: exit value %d\n", lastBackgroundProcessPid, exitCode);
            fflush(stdout);
            lastBackgroundProcessPid = 0;
        }
    }
}

//CODE FROM CLASS MODULES
void handle_SIGINT(int sig)
{
    //set global variables equal to the signal
    status = 0;
    killSignal = sig;

    write(0, "terminated by signal ", 22);
    reentrantWriteInt(sig);
    write(0, "\n", 1);
}

//CODE FROM CLASS MODULES
void handle_SIGTSTP(int sig)
{
    //set global variables equal to the signal
    status = 0;
    killSignal = sig;
    if (foregroundOnlyMode == 0) //switch to foreground only
    {
        foregroundOnlyMode = 1;
    }
    else //exit foreground only
    {
        foregroundOnlyMode = 0;
    }
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

    //separate integers individually to be able to print them with the write() function
    while (max > 0)
    {
        int digit = (value / max) % 10;
        char c = (char)((int)'0' + digit);
        write(0, &c, 1);
        max = max / 10;
    }
}