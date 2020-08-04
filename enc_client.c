/******************************************************************************


*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 30000

const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in *address,
                        int portNumber,
                        char *hostname)
{

    // Clear out the address struct
    memset((char *)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent *hostInfo = gethostbyname(hostname);
    if (hostInfo == NULL)
    {
        fprintf(stderr, "CLIENT: ERROR, no such host\n");
        exit(0);
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char *)&address->sin_addr.s_addr,
           hostInfo->h_addr_list[0],
           hostInfo->h_length);
}

int getFileSize(FILE *file)
{
    //find size of the key file
    fseek(file, 0L, SEEK_END);
    int fileSize = ftell(file);
    fseek(file, 0L, SEEK_SET);

    return fileSize;
}

void writeToSocket(int socketFD, char *buffer)
{
    int charsWritten = send(socketFD, buffer, strlen(buffer), 0);
    while (charsWritten < strlen(buffer))
    {
        if (charsWritten < 0)
        {
            error("CLIENT: ERROR writing to socket");
            exit(1);
        }
        printf("CLIENT: WARNING: Not all data written to socket!\n");
        charsWritten = send(socketFD, buffer, strlen(buffer), 0);
    }
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

void receiveOK(int socketFD)
{
    char buffer[2];
    recv(socketFD, buffer, 2, 0);
}

int main(int argc, char *argv[])
{
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    char buffer[256];
    // Check usage & args
    // if (argc < 3)
    // {
    //     fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
    //     exit(0);
    // }

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0)
    {
        error("CLIENT: ERROR opening socket");
    }

    int port = PORT;
    char *add = "localhost";
    // Set up the server address struct
    setupAddressStruct(&serverAddress, port, add);

    // Connect to server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        error("CLIENT: ERROR connecting");
    }

    char *password = "batman";

    // Send message to server
    // Write to the server
    writeToSocket(socketFD, password);
    receiveOK(socketFD);

    // Get return message from server
    // Clear out the buffer again for reuse
    memset(buffer, '\0', sizeof(buffer));
    // Read data from the socket, leaving \0 at end
    charsRead = readAllChars(socketFD, buffer, 4);
    sendOK(socketFD);

    if (charsRead < 0)
    {
        error("CLIENT: ERROR reading from socket");
    }
    if (strcmp(buffer, "Good") == 0)
    {
        printf("Request accepted from server.\n");
    }
    else
    {
        printf("Request DENIED.\n");
    }

    char *keyFileName = "mykey";
    FILE *keyFile = fopen(keyFileName, "r");
    if (keyFile == NULL)
    {
        fprintf(stderr, "Cannot open the key file: %s.\n", keyFileName);
        exit(1);
    }

    int keyFileSize = getFileSize(keyFile);
    sprintf(buffer, "%10d", keyFileSize);
    writeToSocket(socketFD, buffer);
    receiveOK(socketFD);

    memset(buffer, '\0', sizeof(buffer));
    while (fgets(buffer, 256, keyFile))
    {
        // find '/n', and remove it
        char *lineFeed = strchr(buffer, '\n');
        if (lineFeed)
            *lineFeed = '\0';

        writeToSocket(socketFD, buffer);
        memset(buffer, '\0', sizeof(buffer));
    }
    receiveOK(socketFD);

    char *textFileName = "plaintext";
    FILE *textFile = fopen(textFileName, "r");
    if (textFile == NULL)
    {
        fprintf(stderr, "Cannot open the text file: %s.\n", textFileName);
        exit(1);
    }
    int textFileSize = getFileSize(textFile);
    sprintf(buffer, "%10d", textFileSize);
    writeToSocket(socketFD, buffer);
    receiveOK(socketFD);

    memset(buffer, '\0', sizeof(buffer));
    while (fgets(buffer, 256, textFile))
    {
        // find '/n', and remove it
        char *lineFeed = strchr(buffer, '\n');
        if (lineFeed)
            *lineFeed = '\0';

        writeToSocket(socketFD, buffer);
        memset(buffer, '\0', sizeof(buffer));
    }
    receiveOK(socketFD);

    // Close the socket
    close(socketFD);

    fclose(keyFile);
    fclose(textFile);

    return 0;
}
