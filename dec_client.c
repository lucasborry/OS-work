/******************************************************************************


*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";


char *decode(char *encoded, char *key)
{
    int textLength = strlen(encoded);
    int keyLength = strlen(key);
    char *result = (char *)malloc(sizeof(char) * textLength);
    if (textLength < keyLength)
    {
        for (int i = 0; i < textLength; i++)
        {
            char *found;

            char c = encoded[i];
            found = strchr(alphabet, c);
            int indexText = (int)(found - alphabet);

            char k = key[i];
            found = strchr(alphabet, k);
            int indexKey = (int)(found - alphabet);

            int resultIndex = (27 + (indexText - indexKey)) % 27;
            result[i] = alphabet[resultIndex];
        }
    }
    else
    {
        perror("Invalid, key too short");
        return NULL;
    }
    return result;
}

int main(int argc, char* argv[])
{
    char *encoded = argv[1];
    char *key = argv[2];

    char *decoded = decode(encoded, key);
    if (decoded != NULL)
    {
        printf("%s\n", decoded);
    }

    return 0;
}
