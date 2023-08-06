#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT 9002
#define BUFFER_SIZE 1024


int validate_command(char *command);

int main(int argc, char *argv[]) {
    int client_socket;
    struct sockaddr_in server_addr;
    struct hostent *server;

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t) PORT);//Port number

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) < 0) {
        fprintf(stderr, " inet_pton() has failed\n");
        exit(2);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddrnano *) &server_addr, sizeof(server_addr)) < 0) {//Connect()
        perror("Error connecting to server");
        close(client_socket);
        exit(3);
    }

    char command[BUFFER_SIZE];
    int is_valid;

    while (1) {
        // Read command from the user
        char buff1[1024];
        printf("\nType your message to the server\n");
        scanf("%s", buff1);
        if (strcmp(buff1, "quit") == 0) {
            close(client_socket);
            break;
        }
        write(client_socket, buff1, 1024);

        printf("Message from the server\n");
        char buff2[1024];
        read(client_socket, buff2, 1024);
        printf("%s", buff2);

        // Implement logic to receive and handle responses from the server
    }

}

int validate_command(char *command) {
    // Implement the logic to validate the syntax of the command as per the rules in Section 2.
    // Return 1 if the command is valid, 0 otherwise.
}
