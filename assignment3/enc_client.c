/******************************************************************************
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

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
//send info to server
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

int main(int argc, char *argv[])
{

    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    char buffer[256];

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0)
    {
        error("CLIENT: ERROR opening socket");
    }

    //Check usage & args, need 4 total arguments
    if (argc < 4)
    {
        fprintf(stderr, "Error, too few arguments: [text][key][port].\n");
        exit(2);
    }

    //port number given as the 3rd argument
    int port = atoi(argv[3]);

    char *add = "localhost";
    // Set up the server address struct
    setupAddressStruct(&serverAddress, port, add);

    // Connect to server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        error("CLIENT: ERROR connecting");
    }

    //key file given as the 2nd argument
    char *keyFileName = argv[2];

    //open file containing key
    FILE *keyFile = fopen(keyFileName, "r");
    if (keyFile == NULL)
    {
        fprintf(stderr, "Cannot open the key file: %s.\n", keyFileName);
        exit(1);
    }

    //return key file size -1 to omit last null char
    int keyFileSize = getFileSize(keyFile) - 1;

    char *textFileName = argv[1];

    //open file containing text
    FILE *textFile = fopen(textFileName, "r");
    if (textFile == NULL)
    {
        fprintf(stderr, "Cannot open the text file: %s.\n", textFileName);
        exit(1);
    }

    int textFileSize = getFileSize(textFile) - 1;

    int totalSize = keyFileSize + textFileSize + 1; //account for comma

    //make sure key is bigger than text, error otherwise
    if (keyFileSize < textFileSize)
    {
        error("Error: key is too short\n");
    }

    //unique password to make it impossible to connect to other servers
    char *password = "batman";

    //send info ten chars at a time to not overwhelm the buffer
    sprintf(buffer, "%s,%10d", password, totalSize);

    writeToSocket(socketFD, buffer);

    // Get return message from server
    // Clear out the buffer again for reuse
    memset(buffer, '\0', sizeof(buffer));
    // Read data from the socket, leaving \0 at end
    charsRead = readAllChars(socketFD, buffer, 4); //count all characters read
    if (charsRead < 0)
    {
        error("CLIENT: ERROR reading from socket");
    }
    if (strcmp(buffer, "Good") != 0)
    {
        error("Request DENIED.\n");
    }

    memset(buffer, '\0', sizeof(buffer));
    while (fgets(buffer, 256, keyFile))
    {
        // find '/n', and remove it
        char *lineFeed = strchr(buffer, '\n');
        if (lineFeed)
            *lineFeed = '\0'; //replace by NULL character

        writeToSocket(socketFD, buffer);
        memset(buffer, '\0', sizeof(buffer));
    }
    send(socketFD, ",", 1, 0); //separator between key and text

    memset(buffer, '\0', sizeof(buffer));
    while (fgets(buffer, 256, textFile))
    {

        // find '/n', and remove it
        char *lineFeed = strchr(buffer, '\n');

        if (lineFeed)
            *lineFeed = '\0';

        int i;
        for (i = 0; i < strlen(buffer); i++) //go through the buffer
        {
            if (strchr(alphabet, buffer[i]) == 0) //make sure no weird characters are in the text file
            {

                fprintf(stderr, "Error, wrong character input in %s.\n", textFileName);
                exit(1);
            }
        }
        writeToSocket(socketFD, buffer);
        memset(buffer, '\0', sizeof(buffer));
    }

    int totalRead = 0;
    while (totalRead < textFileSize) //iterate through buffer until we are done
    {
        memset(buffer, '\0', 256);
        int nbCharsRead = recv(socketFD, buffer, 255, 0);
        if (nbCharsRead == -1)
        {
            error("ERROR while reading");
        }
        totalRead = totalRead + nbCharsRead; //add to total characters read
        printf("%s", buffer);                //print out each character from the buffer
    }
    printf("\n");

    // Close the socket
    close(socketFD);

    //close the files
    fclose(keyFile);
    fclose(textFile);

    return 0;
}
