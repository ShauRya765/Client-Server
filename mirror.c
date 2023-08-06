#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 12345
#define BUFFER_SIZE 1024

void processclient(int client_socket);

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pid_t child_pid;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(server_socket);
        exit(1);
    }

    // Listen for client connections
    if (listen(server_socket, 6) < 0) {
        perror("Error listening");
        close(server_socket);
        exit(1);
    }

    int connection_count = 0;
    while (1) {
        // Accept client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Error accepting client connection");
            continue;
        }

        // Count the client connections
        connection_count++;

        // Fork a child process to handle the client request
        child_pid = fork();
        if (child_pid < 0) {
            perror("Error forking child process");
            close(client_socket);
            continue;
        } else if (child_pid == 0) {
            // Child process
            close(server_socket);
            processclient(client_socket);
            close(client_socket);
            exit(0);
        } else {
            // Parent process
            close(client_socket);
        }

        // If 6 connections are handled, break to the alternating mode
        if (connection_count == 6) {
            break;
        }
    }

    // Alternate between server and mirror
    int is_mirror = 0;
    while (1) {
        // Accept client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("Error accepting client connection");
            continue;
        }

        // Fork a child process to handle the client request
        child_pid = fork();
        if (child_pid < 0) {
            perror("Error forking child process");
            close(client_socket);
            continue;
        } else if (child_pid == 0) {
            // Child process
            close(server_socket);

            // Determine if this connection is handled by server or mirror
            if (is_mirror) {
                printf("Connection handled by Mirror\n");
                processclient(client_socket);
            } else {
                printf("Connection handled by Server\n");
                // You can implement the logic for handling client commands for the server here
                // (similar to processclient function in server.c)
            }

            close(client_socket);
            exit(0);
        } else {
            // Parent process
            close(client_socket);
            is_mirror = !is_mirror; // Switch between server and mirror
        }
    }

    close(server_socket);
    return 0;
}

void processclient(int client_socket) {
    // Implementation of the processclient function as per the requirements
    // outlined in the instructions.
    // You can implement the logic for handling client commands here.
    // Don't forget to send appropriate responses to the client based on the commands received.
    // For the mirror, you can follow the same logic as the processclient function in server.c
}
