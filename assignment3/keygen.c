/******************************************************************************
simple program that takes in an argument of keysize to generate a key.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

int main(int argc, char *argv[])
{
    time_t t;
    srand((unsigned)time(&t)); //radom index

    //check that we have the right amount of arguments
    if (argc < 2)
    {
        fprintf(stderr, "Error, too few arguments.\n");
        exit(0);
    }

    char *k = argv[1];
    //keylength given as the argument
    int keyLength = atoi(k);

    int i;
    for (i = 0; i < keyLength; i++) //go until the size of the key
    {
        int r = alphabet[rand() % 27]; //randomize the alphabet
        printf("%c", r);               //print a random letter or space
    }
    printf("\n"); //end line
    return 0;
}