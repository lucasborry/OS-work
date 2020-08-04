#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

int main(int argc, char *argv[])
{
    time_t t;
    srand((unsigned)time(&t));

    if (argc < 2)
    {
        fprintf(stderr, "Error, too few arguments.\n");
        exit(0);
    }

    char *k = argv[1];
    int keyLength = atoi(k);

    for (int i = 0; i < keyLength; i++)
    {
        int r = alphabet[rand() % 27];
        printf("%c", r);
    }
    printf("\n");
    return 0;
}