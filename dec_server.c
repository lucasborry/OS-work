/******************************************************************************


*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

char *decode(char *encoded, char *key)
{
    int textLength = strlen(encoded);
    int keyLength = strlen(key);
    char *result = (char *)malloc(sizeof(char) * textLength);
    if (textLength < keyLength)
    {
        int i;
        for (i = 0; i < textLength; i++)
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

void processNetworkEncoding(int connectionSocket)
{

    int charsRead;
    char buffer[256];
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    charsRead = readAllChars(connectionSocket, buffer, 17);

    char *password = strtok(buffer, ",");
    char *totalSize = strtok(NULL, ",");

    // Get the 'password' from the client, make sure it is the enc_client connecting
    if (strcmp(password, "thanos") == 0)
    {
        int charsSent = send(connectionSocket, "Good", 4, 0);
        if (charsSent != 4)
        {
            error("ERROR while sending");
        }
    }
    else
    {
        int charsSent = send(connectionSocket, "bad ", 4, 0);
        if (charsSent != 4)
        {
            error("ERROR while sending");
        }
    }

    int size = atoi(totalSize);
    char *bigBuffer = (char *)malloc(size);
    readAllChars(connectionSocket, bigBuffer, size);

    char *key = strtok(bigBuffer, ",");
    char *text = strtok(NULL, ",");

    char *decoded = decode(text, key);
    if (decoded != NULL)
    {
        int sizeOfText = strlen(decoded);
        send(connectionSocket, decoded, sizeOfText, 0);
    }
}

pid_t childProcesses[5];
int childProcessesCount()
{
    int count = 0;
    int i;
    for (i = 0; i < 5; i++)
    {
        if (childProcesses[i] != 0)
        {
            count++;
        }
    }
    return count;
}

void removeChildProcess(pid_t processId)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if (childProcesses[i] == processId)
        {
            childProcesses[i] = 0;
        }
    }
}

void addChildProcess(pid_t processId)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if (childProcesses[i] == 0)
        {
            childProcesses[i] = processId;
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    memset(childProcesses, 0, 5 * sizeof(pid_t));
    int connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    int port = atoi(argv[1]);

    if (argc < 2)
    {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

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
        while (childProcessesCount() >= 5)
        {
            sleep(1);
            int exit;
            int i;
            for (i = 0; i < 5; i++)
            {
                pid_t child = childProcesses[i];
                if (child != 0)
                {
                    pid_t childpid = waitpid(child, &exit, WNOHANG); // non-blocking, from modules

                    if (childpid && WIFEXITED(exit))
                    {
                        removeChildProcess(childpid);
                    }
                }
            }
        }

        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0)
        {
            error("ERROR on accept");
        }

        pid_t child = fork();
        if (child == 0)
        {
            processNetworkEncoding(connectionSocket);
            // Close the connection socket for this client
            close(connectionSocket);
            exit(1);
        }
        else
        {
            addChildProcess(child);

            // ajouter child a la liste des child processes
        }
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
