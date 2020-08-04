/******************************************************************************


*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 30000

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

// Error function used for reporting issues
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in *address,
                        int portNumber)
{

    // Clear out the address struct
    memset((char *)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

int readAllChars(int socketFD, char *bufferToFill, int size)
{
    char buffer[256];
    memset(bufferToFill, '\0', size);

    int totalRead = 0;
    while (totalRead < size)
    {
        memset(buffer, '\0', 256);
        int nbCharsRead = recv(socketFD, buffer, 255, 0);
        if (nbCharsRead == -1)
        {
            error("ERROR while reading");
        }
        totalRead = totalRead + nbCharsRead;
        strcat(bufferToFill, buffer);
    }
    return totalRead;
}

void sendOK(int socketFD)
{
    send(socketFD, "OK", 2, 0);
}

int main(int argc, char *argv[])
{
    int connectionSocket, charsRead;
    char buffer[256];
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    int port = PORT;
    // if (argc < 2)
    // {
    //     fprintf(stderr, "USAGE: %s port\n", argv[0]);
    //     exit(1);
    // }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0)
    {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, port);

    // Associate the socket to the port
    if (bind(listenSocket,
             (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) < 0)
    {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1)
    {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0)
        {
            error("ERROR on accept");
        }

        printf("SERVER: Connected to client running at host %d port %d\n",
               ntohs(clientAddress.sin_addr.s_addr),
               ntohs(clientAddress.sin_port));

        // Get the 'password' from the client and display it
        memset(buffer, '\0', 256);
        // Read the client's message from the socket
        charsRead = readAllChars(connectionSocket, buffer, 6);
        sendOK(connectionSocket);

        if (strcmp(buffer, "batman") == 0)
        {
            int charsSent = send(connectionSocket, "Good", 4, 0);
            if (charsSent != 4)
            {
                error("ERROR while sending");
            }
            printf("Good\n");
        }
        else
        {
            int charsSent = send(connectionSocket, "bad ", 4, 0);
            if (charsSent != 4)
            {
                error("ERROR while sending");
            }
            printf("Bad\n");
        }

        readAllChars(connectionSocket, buffer, 10);
        sendOK(connectionSocket);

        int size = atoi(buffer);
        char *key = (char *)malloc(size);
        readAllChars(connectionSocket, key, size);
        sendOK(connectionSocket);

        printf("Key: %s\n", key);

        readAllChars(connectionSocket, buffer, 10);
        sendOK(connectionSocket);

        size = atoi(buffer);
        char *text = (char *)malloc(size);
        readAllChars(connectionSocket, text, size);
        sendOK(connectionSocket);

        printf("Text:    %s\n", text);

        char *encoded = encode(text, key);
        if (encoded != NULL)
        {
            printf("Encoded: %s\n", encoded);
        }
        // Close the connection socket for this client
        close(connectionSocket);
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
