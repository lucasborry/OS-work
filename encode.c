/******************************************************************************


*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

char *encode(char *text, char *key)
{
    int textLength = strlen(text);
    int keyLength = strlen(key);
    char *result = (char *)malloc(sizeof(char) * textLength);
    if (textLength < keyLength)
    {
        for (int i = 0; i < textLength; i++)
        {
            char *found;

            char c = text[i];
            found = strchr(alphabet, c);
            int indexText = (int)(found - alphabet);

            char k = key[i];
            found = strchr(alphabet, k);
            int indexKey = (int)(found - alphabet);

            int resultIndex = (indexText + indexKey) % 27;
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

int main()
{
    // char *text = "ABC DEFGHIJKLM NOPQRSTUV WXZY";
    // char *key = "SDBFUBFUHD UDSFB UHSDFBUHSDBFHUDS SFUHBDSUHF SDUHFBDSUHFSDJBFDSIBFQWI";
    // char *encoded = encode(text, key);
    // if (encoded != NULL)
    // {
    //     printf("%s\n", encoded);
    // }

    char *encoded = "VTEWKMXHAAZELDDQAYGHOITCPBLCRENBAONG";
    char *key = "CMAXUIUIVNLNHEZFTUPIOQURHZZVLYVCJW STIPCVAQREDXMXA";

    char *decoded = decode(encoded, key);
    printf("%5d\n", 4444);
    if (decoded != NULL)
    {
        printf("%s\n", decoded);
    }

    return 0;
}
