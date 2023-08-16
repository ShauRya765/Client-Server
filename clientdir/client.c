#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>

#define PORT 9003
#define BUFFER_SIZE 2048
int isCmdValid = 0;


void validate_input_command(char *command);

void receive_tar_file(int socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received = 0;
    int total_received = 0;

    int file_size;
    if (recv(socket, &file_size, sizeof(file_size), 0) <= 0) {
        perror("Error receiving file size");
        return;
    }

    FILE *fp = fopen("received.tar.gz", "wb");
    if (fp == NULL) {
        perror("Error opening file");
        return;
    }
    printf("%d\n", file_size);

    while (total_received < file_size) {
        int remaining_bytes = file_size - total_received;
        int chunk_size = remaining_bytes < BUFFER_SIZE ? remaining_bytes : BUFFER_SIZE;

        bytes_received = recv(socket, buffer, chunk_size, 0);
        if (bytes_received <= 0) {
            perror("Error in recv");
            fclose(fp);
            return;
        }

        fwrite(buffer, 1, bytes_received, fp);
        total_received += bytes_received;
    }
    printf("%d\n", total_received);
    fclose(fp);

    memset(buffer, 0, sizeof(buffer));
    // Extract the received tar.gz file
    // int extraction_result = extract_tar_gz(filename);
    // if (extraction_result == 0) {
    //     printf("File extracted successfully.\n");
    // } else {
    //     printf("File extraction failed.\n");
    // }
}


void receive_response(int socket) {
    char response[BUFFER_SIZE];
    ssize_t bytes_received = recv(socket, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("Response from server: %s\n", response);
    } else if (bytes_received == 0) {
        printf("Connection closed by the server.\n");
    } else {
        perror("Error receiving response");
    }
}


int main(int argc, char *argv[]) {
    int client_sock_sd;
    struct sockaddr_in server_ip;

    // Creating client socket
    client_sock_sd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock_sd < 0) {
        perror("Error creating socket for client, try again");
        exit(1);
    }

    memset(&server_ip, 0, sizeof(server_ip));
    server_ip.sin_family = AF_INET;
    server_ip.sin_port = htons((uint16_t) atoi(argv[2]));//Port number

    if (inet_pton(AF_INET, argv[1], &server_ip.sin_addr) < 0) {
        fprintf(stderr, " inet_pton() has failed\n");
        exit(2);
    }

    // Connecting to the server using server ip
    if (connect(client_sock_sd, (struct sockaddr *) &server_ip, sizeof(server_ip)) < 0) {//Connect()
        perror("Error connecting to server, try again");
        close(client_sock_sd);
        exit(3);
    }
    int isMirror;
    recv(client_sock_sd, &isMirror, sizeof(isMirror), 0);

    // Infinite loop to run commands
    while (1) {
        int client_mirror_sd;
        if (isMirror == 1) {
            close(client_sock_sd);
            printf("true :: => :: %d\n", isMirror);
            struct sockaddr_in mirror_server_ip;

            // Creating client socket
            client_mirror_sd = socket(AF_INET, SOCK_STREAM, 0);
            if (client_mirror_sd < 0) {
                perror("Error creating socket for client, try again");
                exit(1);
            }

            memset(&mirror_server_ip, 0, sizeof(mirror_server_ip));
            mirror_server_ip.sin_family = AF_INET;
            mirror_server_ip.sin_port = htons((uint16_t) atoi(argv[3]));//Port number

            if (inet_pton(AF_INET, argv[1], &mirror_server_ip.sin_addr) < 0) {
                fprintf(stderr, " inet_pton() has failed\n");
                exit(2);
            }

            // Connecting to the server using server ip
            if (connect(client_mirror_sd, (struct sockaddr *) &mirror_server_ip, sizeof(mirror_server_ip)) <
                0) {//Connect()
                perror("Error connecting to server, try again");
                close(client_mirror_sd);
                exit(3);
            }
            char cmdArr[BUFFER_SIZE];        // Reading command from the user
            printf("\nEnter Command:\n");
            fgets(cmdArr, sizeof(cmdArr), stdin);

            // Remove the newline character from the end of the input
            size_t input_length = strlen(cmdArr);
            if (input_length > 0 && cmdArr[input_length - 1] == '\n') {
                cmdArr[input_length - 1] = '\0';
            }

            char tempCmdArr[BUFFER_SIZE];
            strcpy(tempCmdArr, cmdArr);

            // validating input command using validate_input_command function
            validate_input_command(tempCmdArr);

            // check if input command is valid or not according to given requirements
            if (isCmdValid == 1) {
                // sending the command to server socket
                send(client_mirror_sd, cmdArr, strlen(cmdArr), 0);

                // checking if input cmd is quit or not
                if (strcmp(cmdArr, "quit") == 0) {
                    close(client_mirror_sd);
                    break;
                }

                printf("Message from the server:\n\n");

                // Receive the flag from the server
                int flag;
                recv(client_mirror_sd, &flag, sizeof(flag), 0);
                printf("flag :: => :: %d\n", flag);

                if (flag == 1) {
                    receive_tar_file(client_mirror_sd);
                } else {
                    char server_response[BUFFER_SIZE];
                    recv(client_mirror_sd, server_response, sizeof(server_response), 0);
                    printf("%s", server_response);
                    memset(server_response, 0, sizeof(server_response));
                }
            } else {
                printf("\ncommand is not valid\n");
            }
        } else {
            char cmdArr[BUFFER_SIZE];        // Reading command from the user
            printf("\nEnter Command:\n");
            fgets(cmdArr, sizeof(cmdArr), stdin);
            printf("server true :: => :: ");
            // Remove the newline character from the end of the input
            size_t input_length = strlen(cmdArr);
            if (input_length > 0 && cmdArr[input_length - 1] == '\n') {
                cmdArr[input_length - 1] = '\0';
            }

            char tempCmdArr[BUFFER_SIZE];
            strcpy(tempCmdArr, cmdArr);

            // validating input command using validate_input_command function
            validate_input_command(tempCmdArr);

            /// check if input command is valid or not according to given requirements
            if (isCmdValid == 1) {
                // sending the command to server socket
                send(client_sock_sd, cmdArr, strlen(cmdArr), 0);

                // checking if input cmd is quit or not
                if (strcmp(cmdArr, "quit") == 0) {
                    close(client_sock_sd);
                    break;
                }

                printf("Message from the server:\n\n");

                // Receive the flag from the server
                int flag;
                recv(client_sock_sd, &flag, sizeof(flag), 0);
                printf("flag :: => :: %d\n", flag);

                if (flag == 1) {
                    receive_tar_file(client_sock_sd);
                } else {
                    char server_response[BUFFER_SIZE];
                    recv(client_sock_sd, server_response, sizeof(server_response), 0);
                    printf("%s", server_response);
                    memset(server_response, 0, sizeof(server_response));
                }
            } else {
                printf("\ncommand is not valid\n");
            }
        }
    }
}

// function to count number of extensions
int checkInputCmd(char *command) {
    int fileCount = 0;
    char *token = strtok(command, " ");
    while (token != NULL) {
        fileCount++;
        token = strtok(NULL, " ");
    }
    return fileCount;
}

// function to check is substring is present in the string or not
int substrExists(const char *command, const char *substring) {
    int commandLen = strlen(command);       // Length of the command string
    int substringLen = strlen(substring);   // Length of the substring to be found

    // Loop through the command string to check for the presence of the substring
    for (int i = 0; i <= commandLen - substringLen; i++) {
        int j;
        // Compare each character in the command string with the substring
        for (j = 0; j < substringLen; j++) {
            if (command[i + j] != substring[j])
                break; // If characters don't match, break out of the inner loop
        }

        // If the inner loop completes without a break, the entire substring has been found
        if (j == substringLen)
            return 1; // Return 1 to indicate that the substring exists in the command
    }

    // If the loop completes without finding the substring, return 0
    return 0; // Return 0 to indicate that the substring does not exist in the command
}

// function to validate fgets cmd
void validate_fGets(char *cmd) {
    int c = checkInputCmd(cmd);
    if (c <= 5) {
        isCmdValid = 1;
    } else {
        isCmdValid = 0;
    }
}

// function to validate tarfgets cmd
void validate_tarGets(char *cmd) {
    int size1, size2;
    char flag[5];
    if (sscanf(cmd, "tarfgetz %d %d %4s", &size1, &size2, flag) == 3) {
        if (strcmp(flag, "-u") == 0) {
            if (size1 <= size2) {
                isCmdValid = 1;
            } else {
                isCmdValid = 0;
            }
        } else {
            isCmdValid = 0;
        }
    } else if (sscanf(cmd, "tarfgetz %d %d", &size1, &size2) == 2) {
        if (size1 <= size2 && size1 > 0 && size2 > 0) {
            isCmdValid = 1;
        } else {
            isCmdValid = 0;
        }
    } else {
        isCmdValid = 0;
        printf("Invalid format, usage: tarfgetz size1 size2 <-u>");
    }
}

// function to validate date
bool is_valid_date(const char *date) {
    // Check for valid format (YYYY-MM-DD)
    if (strlen(date) != 10 || date[4] != '-' || date[7] != '-') {
        return false;
    }

    // Check that the non-dash characters are valid digits
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            continue;  // Skip dashes
        }
        if (date[i] < '0' || date[i] > '9') {
            return false;
        }
    }
    // Parse the year, month, and day from the date string
    int year = atoi(date);
    int month = atoi(date + 5);
    int day = atoi(date + 8);

    // Check if the parsed values are within valid ranges
    if (year < 1000 || year > 9999 || month < 1 || month > 12 || day < 1 || day > 31) {
        return 0;
    }

    // Additional checks for specific months with fewer days
    if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
        return 0;
    }

    // Check for February with leap year
    if (month == 2) {
        int is_leap_year = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if ((is_leap_year && day > 29) || (!is_leap_year && day > 28)) {
            return 0;
        }
    }

    // Additional validation logic can be added here if needed

    return true;
}

// function to validate getdirf cmd
void validate_getDirf(char *cmd) {
    char date1[11], date2[11]; // Room for YYYY-MM-DD and null-terminator
    char flag[5];
    if (sscanf(cmd, "getdirf %10s %10s %4s", date1, date2, flag) == 3) {
        if (strcmp(flag, "-u") == 0) {
            if (is_valid_date(date1) && is_valid_date(date2)) {
                if (strcmp(date1, date2) <= 0) {
                    isCmdValid = 1;
                } else {
                    isCmdValid = 0;
                }
            } else {
                isCmdValid = 0;
            }
        } else {
            isCmdValid = 0;
            printf("Invalid format, usage: tarfgetz size1 size2 <-u>");
        }
    } else if (sscanf(cmd, "getdirf %10s %10s", date1, date2) == 2) {
        if (is_valid_date(date1) && is_valid_date(date2)) {
            if (strcmp(date1, date2) <= 0) {
                isCmdValid = 1;
            } else {
                isCmdValid = 0;
            }
        } else {
            isCmdValid = 0;
        }
    } else {
        isCmdValid = 0;
        printf("Invalid format, usage: tarfgetz size1 size2 <-u>");
    }
}

// function to validate input cmds
void validate_input_command(char *input_cmd) {
    char *tempCmd = input_cmd;
    if (substrExists(tempCmd, "fgets")) {
        validate_fGets(input_cmd);
    } else if (substrExists(tempCmd, "tarfgetz")) {
        validate_tarGets(tempCmd);
    } else if (substrExists(tempCmd, "filesrch")) {
        isCmdValid = 1;
    } else if (substrExists(tempCmd, "targzf")) {
        isCmdValid = 1;
    } else if (substrExists(tempCmd, "getdirf")) {
        validate_getDirf(input_cmd);
    } else if (substrExists(tempCmd, "quit")) {
        isCmdValid = 1;
    } else {
        isCmdValid = 0;
    }
}


