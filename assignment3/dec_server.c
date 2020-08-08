/******************************************************************************
Handle everything given by the client. Take in a key and text to decode.
Sends the decoded text back to the client.
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
    int textLength = strlen(encoded);                         //save encoded text length
    int keyLength = strlen(key);                              //save key length
    char *result = (char *)malloc(sizeof(char) * textLength); //allocate memory for resulting cipher
    if (textLength < keyLength)
    {
        int i;
        for (i = 0; i < textLength; i++)
        {
            char *found;

            char c = encoded[i];
            found = strchr(alphabet, c);             //put that char we found into our char*
            int indexText = (int)(found - alphabet); //save the index

            char k = key[i];
            found = strchr(alphabet, k);            //put that key we found into our char*
            int indexKey = (int)(found - alphabet); //save the index

            int resultIndex = (27 + (indexText - indexKey)) % 27; //our resulting index randomized
            result[i] = alphabet[resultIndex];                    //put that resulting char into our result each iteration of the loop to decode
        }
    }
    else
    {
        fprintf(stderr, "Error: key %s is too short.\n", key); //error message if key is too short
        return NULL;
    }
    return result; //return our resulting array of char* decoded text
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

//this functiong takes in a buffer and its size, reads the buffer, and returns the total read
int readAllChars(int socketFD, char *bufferToFill, int size)
{
    char buffer[256];
    memset(bufferToFill, '\0', size); //set new buffer to 0

    int totalRead = 0;
    while (totalRead < size) //while we have not gone through the whole thing
    {
        memset(buffer, '\0', 256);
        int nbCharsRead = recv(socketFD, buffer, 255, 0); //receive info from server
        if (nbCharsRead == -1)
        {
            error("ERROR while reading");
        }
        totalRead = totalRead + nbCharsRead; //add up total read and nb of chars
        strcat(bufferToFill, buffer);        //copy elements into buffer given as an argument
    }
    return totalRead;
}

//process all informations given by the client
void processNetworkDecoding(int connectionSocket)
{
    int charsRead;
    char buffer[256];
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    charsRead = readAllChars(connectionSocket, buffer, 17);

    //use strtok to separate password from the rest of the info
    char *password = strtok(buffer, ",");
    char *totalSize = strtok(NULL, ",");

    // Get the 'password' from the client, make sure it is the dec_client connecting
    if (strcmp(password, "thanos") == 0)
    {
        int charsSent = send(connectionSocket, "Good", 4, 0); //send validation message back to client
        if (charsSent != 4)
        {
            error("ERROR while sending");
        }
    }
    else
    {
        int charsSent = send(connectionSocket, "bad ", 4, 0); //send rejection message back to client
        if (charsSent != 4)
        {
            error("ERROR while sending");
        }
    }

    int size = atoi(totalSize);             //find size given in the buffer
    char *bigBuffer = (char *)malloc(size); //allocate memory for a bigger buffer to put everything in
    readAllChars(connectionSocket, bigBuffer, size);

    //key is before the comma
    char *key = strtok(bigBuffer, ",");
    //text is after the comma
    char *text = strtok(NULL, ",");

    char *decoded = decode(text, key);
    if (decoded != NULL)
    {
        int sizeOfText = strlen(decoded); //find size of the text to send back info to client
        send(connectionSocket, decoded, sizeOfText, 0);
    }
}

pid_t childProcesses[5]; //allocate an array for the child processes
//function to count each process in the array
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

//function to remove a child process when array is full
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

//function to add a child process when requested
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
    memset(childProcesses, 0, 5 * sizeof(pid_t)); //set all of the array to 0s
    int connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    int port = atoi(argv[1]); //get port input

    // Check usage & args
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
        while (childProcessesCount() >= 5) //while our array of childpid's isn't full
        {
            sleep(1);
            int exit;
            int i;
            for (i = 0; i < 5; i++)
            {
                pid_t child = childProcesses[i];
                if (child != 0) //if we are not in a child process
                {
                    pid_t childpid = waitpid(child, &exit, WNOHANG); // non-blocking, from modules

                    if (childpid && WIFEXITED(exit)) //exit code given
                    {
                        removeChildProcess(childpid); //remove that child
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

        pid_t child = fork(); //fork off a child
        if (child == 0)
        {
            processNetworkDecoding(connectionSocket);
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
