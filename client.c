// Client program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, n, user_id, group_id;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 5)
    {
        fprintf(stderr, "Usage: %s <user_id> <group_id> <file_name> <file_path>\n", argv[0]);
        exit(1);
    }

    user_id = atoi(argv[1]);
    group_id = atoi(argv[2]);

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    // Set up the server address structure
    server = gethostbyname(SERVER_IP);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(1);
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(SERVER_PORT);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // Send user ID and group ID to the server
    n = write(sockfd, &user_id, sizeof(user_id));
    if (n < 0)
        error("ERROR writing user ID to socket");

    n = write(sockfd, &group_id, sizeof(group_id));
    if (n < 0)
        error("ERROR writing group ID to socket");

    // Send the file name to the server
    n = write(sockfd, argv[3], strlen(argv[3]));
    if (n < 0)
        error("ERROR writing file name to socket");

    // Open the file for reading
    int fd = open(argv[4], O_RDONLY);
    if (fd < 0)
        error("ERROR opening file");

    // Read the file data and send it to the server
    char buffer[256];
    while ((n = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (write(sockfd, buffer, n) != n)
            error("ERROR writing file data to socket");
    }
    if (n < 0)
        error("ERROR reading file");

    // Close the file
    close(fd);

    // Read the server's response
    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0)
        error("ERROR reading from socket");
    printf("%s\n", buffer);

    // Close the socket
    close(sockfd);
    return 0;
}
