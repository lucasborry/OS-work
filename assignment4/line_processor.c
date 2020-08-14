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
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize the condition variables
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

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
    size_t len = 0;
    char *buffer = (char *)malloc(1000 * sizeof(char));
    ssize_t size = getline(&buffer, &len, stdin);
    int indexToWrite = strlen(buf1);
    int i = 0;
    for (i = 0; i <= size; i++)
    {
        buf1[indexToWrite + i] = buffer[i];
    }
}

void *transform(void *args)
{
    int buf2Size = strlen(buf2);
    int i = 0;
    for (i = 0; i <= strlen(buf1); i++)
    {
        buf2[buf2Size + i] = toupper(buf1[i]);
        if (buf2[buf2Size + i] == '\n')
        {
            buf2[buf2Size + i] = ' ';
        }
    }
    buf1[0] = 0;
}
void *output(void *args)
{
    int bufferSize = strlen(buf2);
    int i = 0;

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
}

int main(int argc, char *argv[])
{
    void *args;

    buf1[0] = 0;
    buf2[0] = 0;
    printf("\n");

    while (1)
    {

        input(args);
        transform(args);
        output(args);

        // Create the producer and consumer threads
        pthread_t p, c, i, ls, ps, o;
        //pthread_create(&p, NULL, producer, NULL);
        // Sleep for a few seconds to allow the producer to fill up the buffer. This has been put in to demonstrate the the producer blocks when the buffer is full. Real-world systems won't have this sleep
        //pthread_create(&c, NULL, consumer, NULL);

        pthread_create(&i, NULL, input, NULL);
        // pthread_create(&ls, NULL, lineSeparator, NULL);
        // pthread_create(&ps, NULL, plusSign, NULL);
        // pthread_create(&o, NULL, output, NULL);

        pthread_join(i, NULL);
        // pthread_join(ls, NULL);
        // pthread_join(ps, NULL);
        // pthread_join(o, NULL);

        //pthread_join(p, NULL);
        //pthread_join(c, NULL);

        return 0;
    }