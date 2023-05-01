#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

// Define constants for the directories
#define MANUFACTURING_DIR "Manufacturing/"
#define DISTRIBUTION_DIR "Distribution/"

#define MANUFACTURING_USER_ID 1001
#define DISTRIBUTION_USER_ID 1002
#define MANUFACTURING_GROUP_ID 2001
#define DISTRIBUTION_GROUP_ID 2002

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void *client_handler(void *client_sockfd)
{
    printf("Handeling Client");
    int sockfd = *((int *)client_sockfd);
    char buffer[256];
    char file_name[256];
    int n, user_id, group_id;
    // Read user ID and group ID from the client
    n = read(sockfd, &user_id, sizeof(user_id));
    if (n < 0)
        error("ERROR reading user ID from socket");

    n = read(sockfd, &group_id, sizeof(group_id));
    if (n < 0)
        error("ERROR reading group ID from socket");

    // Set the real and effective user and group IDs for the server process
    if (setregid(group_id, group_id) < 0)
        error("ERROR setting real and effective group IDs");
    if (setreuid(user_id, user_id) < 0)
        error("ERROR setting real and effective user IDs");

    // Read the file name from the client
    bzero(file_name, 256);
    n = read(sockfd, file_name, 255);
    if (n < 0)
        error("ERROR reading file name from socket");

    // Determine the directory based on the group ID
    char directory[256];
    if (group_id == MANUFACTURING_GROUP_ID)
    {
        strcpy(directory, MANUFACTURING_DIR);
    }
    else if (group_id == DISTRIBUTION_GROUP_ID)
    {
        strcpy(directory, DISTRIBUTION_DIR);
    }
    else
    {
        error("Invalid group ID");
    }

    // Create the full file path
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "%s%s", directory, file_name);

    // Open the file for writing
    int fd = open(file_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
        error("ERROR opening file");

    // Read the file data from the client and write it to the file
    while ((n = read(sockfd, buffer, sizeof(buffer))) > 0)
    {
        if (write(fd, buffer, n) != n)
            error("ERROR writing to file");
    }
    if (n < 0)
        error("ERROR reading file data from socket");

    // Change the file's owner to the user ID
    if (fchown(fd, user_id, -1) < 0)
        error("ERROR changing file owner");

    // Close the file
    close(fd);

    // Send a success message to the client
    n = write(sockfd, "File transfer successful", 24);
    if (n < 0)
        error("ERROR writing to socket");

    // Reset the real and effective user and group IDs to root (or another privileged user) before exiting
    if (setregid(0, 0) < 0)
        error("ERROR resetting real and effective group IDs");
    if (setreuid(0, 0) < 0)
        error("ERROR resetting real and effective user IDs");

    // Close the client socket and exit the thread
    close(sockfd);
    pthread_exit(NULL);
}

int main()
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    // Clear the server address structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = SERVER_PORT;

    // Set up the server address structure
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(portno);

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // Listen for incoming client connections
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1)
    {
        printf("Creating Thread");
        // Accept incoming client connections
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        // Create a new thread to handle the client connection
        pthread_t client_thread;
        printf("Creating Thread");
        int thread_status = pthread_create(&client_thread, NULL, client_handler, (void *)&newsockfd);
        if (thread_status != 0)
        {
            fprintf(stderr, "Error creating thread: %d\n", thread_status);
            exit(1);
        }

        // Detach the thread to automatically release resources when the thread exits
        pthread_detach(client_thread);
    }

    // Close the server socket
    close(sockfd);
    return 0;
}
