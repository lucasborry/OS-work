#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

// Size of the buffer
#define SIZE 5

// Special marker used to indicate end of the producer data
#define END_MARKER -1

// Buffer, shared resource
int buffer[SIZE];
char buf1[10000];
char buf2[10000];
// Initialize the mutex
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// Initialize the condition variables
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

/*
 Produce an item. Produces a random integer between [0, 1000] unless it is the last item to be produced in which case the value -1 is returned.
*/

void *plusSign(void *args)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        printf("plusSign thread: %d\n", i);
        sleep(1);
    }
}
void *lineSeparator(void *args)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        printf("lineSeparator thread: %d\n", i);
        sleep(1);
    }
}

void *input(void *args)
{
    while (1)
    {

        size_t len = 0;
        char *userInput;
        ssize_t size = getline(&userInput, &len, stdin);
        int indexToWrite = strlen(buf1);
        int i = 0;

        // Lock the mutex
        pthread_mutex_lock(&mutex1);

        for (i = 0; i <= size; i++)
        {
            buf1[indexToWrite + i] = userInput[i];
        }
        free(userInput);
        // Signal to the consumer that the buffer is no longer empty
        pthread_cond_signal(&cond1);
        // Unlock the mutex
        pthread_mutex_unlock(&mutex1);
    }
}

void *transform(void *args)
{
    while (1)
    {

        int i = 0;

        pthread_mutex_lock(&mutex1);
        while (strlen(buf1) == 0)
        {
            pthread_cond_wait(&cond1, &mutex1);
        }
        pthread_mutex_lock(&mutex2);

        int buf2Size = strlen(buf2);

        for (i = 0; i <= strlen(buf1); i++)
        {
            buf2[buf2Size + i] = toupper(buf1[i]);
            if (buf2[buf2Size + i] == '\n')
            {
                buf2[buf2Size + i] = ' ';
            }
        }
        buf1[0] = 0;

        pthread_cond_signal(&cond2);
        pthread_mutex_unlock(&mutex1);
        pthread_mutex_unlock(&mutex2);
    }
}

void *output(void *args)
{
    while (1)
    {
        int i = 0;
        pthread_mutex_lock(&mutex2);

        while (strlen(buf2) == 0)
        {
            pthread_cond_wait(&cond2, &mutex2);
        }
        int bufferSize = strlen(buf2);

        while (bufferSize >= 80)
        {
            for (i = 0; i < 80; i++)
            {
                printf("%c", buf2[i]);
            }
            printf("\n");
            for (i = 80; i <= bufferSize; i++)
            {
                buf2[i - 80] = buf2[i];
            }
            bufferSize = strlen(buf2);
        }

        // Unlock the mutex
        pthread_mutex_unlock(&mutex2);
    }
}

int main(int argc, char *argv[])
{
    void *args;

    buf1[0] = 0;
    buf2[0] = 0;
    printf("\n");

    // Create the producer and consumer threads
    pthread_t i, t, o;

    pthread_create(&i, NULL, input, NULL);
    pthread_create(&t, NULL, transform, NULL);
    pthread_create(&o, NULL, output, NULL);

    pthread_join(i, NULL);
    pthread_join(t, NULL);
    pthread_join(o, NULL);

    return 0;
}