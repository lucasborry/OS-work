#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>

//define max input for buffers
#define MAX_INPUT 1000

pthread_mutex_t lineSeparatorMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lineSeparatorWritten = PTHREAD_COND_INITIALIZER;
char lineSeparatorBuffer[10 * MAX_INPUT]; // specs: 10x max input

pthread_mutex_t plusSignMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t plusSignWritten = PTHREAD_COND_INITIALIZER;
char plusSignBuffer[10 * MAX_INPUT]; // specs: 10x max input

pthread_mutex_t outputMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t outputWritten = PTHREAD_COND_INITIALIZER;
char outputBuffer[10 * MAX_INPUT]; // specs: 10x max input

//function used to debug, trace errors
#define TRACE 0
void trace(const char *message)
{
#if TRACE
    printf(message);
#endif
}

//bad for performance speed but useful to not repeat
//adds a char in at a given index. We want to add in order to not override existing values in a given buffer.
void addChar(char *string, char toAdd)
{
    int index = strlen(string);
    string[index] = toAdd;
    string[index + 1] = '\0';
}
//run when 1, stop at 0
int run = 1;
void *inputThread(void *args)
{
    //Input Thread: This thread performs input on a line-by-line basis from standard input.
    do
    {
        trace("   ***inputThread looping\n"); //debug function
        char *inputBuffer = NULL;
        size_t len = 0;
        getline(&inputBuffer, &len, stdin);               //get user input
        if (pthread_mutex_lock(&lineSeparatorMutex) == 0) //lock to affect buffer
        {
            if (strcmp(inputBuffer, "DONE\n") == 0) //if it says DONE followed by end line
            {
                run = 0;                          //stop running if we get DONE
                strcat(lineSeparatorBuffer, "@"); //add a special char at the end of DONE: i.e: DONE@
            }
            else
                strcat(lineSeparatorBuffer, inputBuffer); //else copy elements of initial buffer into next buffer
            free(inputBuffer);                            //free old buffer

            pthread_cond_signal(&lineSeparatorWritten); //signal that the buffer is ready to be affected in lineSeparatorThread
            pthread_mutex_unlock(&lineSeparatorMutex);  //unlock buffer
        }
    } while (run);
    return NULL;
}

void *lineSeparatorThread(void *args)
{
    // Line Separator Thread: This thread replaces line separators with blanks.
    do
    {
        trace("   ***lineSeparatorThread looping\n");     //debug function
        if (pthread_mutex_lock(&lineSeparatorMutex) == 0) //lock to affect line separator buffer
        {
            while (strlen(lineSeparatorBuffer) == 0)                           //while the buffer is not empty
                pthread_cond_wait(&lineSeparatorWritten, &lineSeparatorMutex); //wait until signal has been given by the lineSeparator mutex
            if (pthread_mutex_lock(&plusSignMutex) == 0)                       //lock to affect buffer
            {
                int i = 0;                                        //iteration counter
                for (i = 0; i < strlen(lineSeparatorBuffer); i++) //until we have iterated through the buffer
                {
                    if (lineSeparatorBuffer[i] == '\n') //replace end lines by spaces
                        addChar(plusSignBuffer, ' ');
                    else
                        addChar(plusSignBuffer, lineSeparatorBuffer[i]); //add each element of lineSeparatorBuffer into plusSignBuffer
                }
                lineSeparatorBuffer[0] = 0; //clean buffer

                pthread_cond_signal(&plusSignWritten); //signal to next buffer that the process is finished here
                pthread_mutex_unlock(&plusSignMutex);  //unlock mutex for next step
            }
            pthread_mutex_unlock(&lineSeparatorMutex); //unlock mutex for next iteration of the while loop
        }
    } while (run);
    return NULL;
}

void *plusSignThread(void *args)
{
    // Plus Sign Thread: This thread performs the required replacement of pair of plus signs.
    do
    {
        trace("   ***plusSignThread looping\n");     //debug function
        if (pthread_mutex_lock(&plusSignMutex) == 0) //lock to affect line separator buffer
        {
            while (strlen(plusSignBuffer) == 0)                      //while the buffer is not empty
                pthread_cond_wait(&plusSignWritten, &plusSignMutex); //wait until signal has been given by the plusSign mutex
            if (pthread_mutex_lock(&outputMutex) == 0)               //lock to affect buffer
            {
                int i = 0;                                   //iteration counter
                for (i = 0; i < strlen(plusSignBuffer); i++) //until we have iterated through the buffer
                {
                    if (plusSignBuffer[i] == '+' && plusSignBuffer[i + 1] == '+') //if the char is a + and the next one is also a +
                    {
                        addChar(outputBuffer, '^'); //replace by one ^
                        i++;                        //increment one bmore because we don't want to iterate over the 2nd +
                    }
                    else
                        addChar(outputBuffer, plusSignBuffer[i]); //add each element of lineSeparatorBuffer into outputSignBuffer
                }
                plusSignBuffer[0] = 0;               //clean buffer
                pthread_cond_signal(&outputWritten); //signal to next buffer that the process is finished here
                pthread_mutex_unlock(&outputMutex);  //unlock mutex for next step
            }
            pthread_mutex_unlock(&plusSignMutex); //unlock mutex for next iteration of the while loop
        }
    } while (run);
    return NULL;
}

void *outputThread(void *args)
{
    // Output Thread: This thread writes output lines to the standard output.
    do
    {
        trace("   ***outputThread looping\n"); //debug function
        if (pthread_mutex_lock(&outputMutex) == 0)
        {
            while (strlen(outputBuffer) < 80 && run == 1)                  //while it is running and the buffer is not full
                pthread_cond_wait(&outputWritten, &outputMutex);           //wait until signal has been given by the plusSign mutex
            if (run == 0 && outputBuffer[strlen(outputBuffer) - 1] == '@') //if we are done running and there is an @
                outputBuffer[strlen(outputBuffer) - 1] = '\0';             //repace @ by NULL character
            while (strlen(outputBuffer) >= 80)                             //while our buffer is still filled more than 80 chars
            {
                char buffer[81];                             //new buffer
                strncpy(buffer, outputBuffer, 80);           //copy 80 characters of old buffer into new buffer
                buffer[80] = 0;                              //put a null at 80th element
                printf("%s\n", buffer);                      //print content (80 characters) then end line
                int i = 0;                                   //iteration counter
                for (i = 80; i <= strlen(outputBuffer); i++) //until we have iterated through the buffer
                    outputBuffer[i - 80] = outputBuffer[i];  //fill buffer
            }
            pthread_mutex_unlock(&outputMutex); //unlock mutex
        }
    } while (run);
    return NULL;
}

int main(int argc, char *argv[])
{
    plusSignBuffer[0] = 0;
    lineSeparatorBuffer[0] = 0;
    outputBuffer[0] = 0;

    pthread_t inputTid;
    pthread_create(&inputTid, NULL, inputThread, NULL); //create input thread

    pthread_t lineSeparatorTid;
    pthread_create(&lineSeparatorTid, NULL, lineSeparatorThread, NULL); //create line separator thread

    pthread_t plusSignTid;
    pthread_create(&plusSignTid, NULL, plusSignThread, NULL); //create plus sign thread

    pthread_t outputTid;
    pthread_create(&outputTid, NULL, outputThread, NULL); //create output thread

    pthread_join(inputTid, NULL);
    pthread_join(plusSignTid, NULL);
    pthread_join(lineSeparatorTid, NULL);
    pthread_join(outputTid, NULL);

    return 0;
}
