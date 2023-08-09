#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <tar.h>


#define PORT 9002
#define BUFFER_SIZE 1024

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char typeflag[1];
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    // ...
};

void processclient(int client_socket, pid_t pro_id);

// ------------------------------------- validate_command -------------------------------

int is_valid_date_format(const char *date_str) {
    // Check if the date string is not NULL and has the correct length (10 characters for yyyy-mm-dd)
    if (date_str == NULL || strlen(date_str) != 10) {
        return 0;
    }

    // Check if each character in the date string is a digit or a hyphen
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            // The 5th and 8th characters should be hyphens
            if (date_str[i] != '-') {
                return 0;
            }
        } else {
            // All other characters should be digits
            if (!isdigit(date_str[i])) {
                return 0;
            }
        }
    }

    // Parse the year, month, and day from the date string
    int year = atoi(date_str);
    int month = atoi(date_str + 5);
    int day = atoi(date_str + 8);

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

    // The date format is valid
    return 1;
}


int validate_command(char *command) {
    // Tokenize the command to extract the command type and arguments
    char *command_type = strtok(command, " ");
    char *arguments = strtok(NULL, "");

    if (strcmp(command_type, "fgets") == 0) {
        // Check syntax for 'fgets' command: fgets file1 file2 file3 file4
        // The command must start with 'fgets' followed by at least one filename.
        if (arguments == NULL) {
            return 0;
        }
    } else if (strcmp(command_type, "tarfgetz") == 0) {
        // Check syntax for 'tarfgetz' command: tarfgetz size1 size2 <-u>
        // The command must start with 'tarfgetz' followed by two integers (size1 and size2).
        // An optional '-u' flag can be included at the end.
        char *size1_str = strtok(arguments, " ");
        char *size2_str = strtok(NULL, " ");
        char *unzip_flag = strtok(NULL, " ");
        if (size1_str == NULL || size2_str == NULL) {
            return 0;
        }
        int size1 = atoi(size1_str);
        int size2 = atoi(size2_str);
        if (size1 < 0 || size2 < 0 || size1 > size2) {
            return 0;
        }
        if (unzip_flag != NULL && strcmp(unzip_flag, "-u") != 0) {
            return 0;
        }
    } else if (strcmp(command_type, "filesrch") == 0) {
        // Check syntax for 'filesrch' command: filesrch filename
        // The command must start with 'filesrch' followed by a filename.
        if (arguments == NULL) {
            return 0;
        }
    } else if (strcmp(command_type, "targzf") == 0) {
        // Check syntax for 'targzf' command: targzf <extension list> <-u>
        // The command must start with 'targzf' followed by at least one file extension.
        // An optional '-u' flag can be included at the end.
        char *extensions = strtok(arguments, " ");
        char *unzip_flag = strtok(NULL, " ");
        if (extensions == NULL) {
            return 0;
        }
        if (unzip_flag != NULL && strcmp(unzip_flag, "-u") != 0) {
            return 0;
        }
    } else if (strcmp(command_type, "getdirf") == 0) {
        // Check syntax for 'getdirf' command: getdirf date1 date2 <-u>
        // The command must start with 'getdirf' followed by two dates in yyyy-mm-dd format.
        // An optional '-u' flag can be included at the end.
        char *date1 = strtok(arguments, " ");
        char *date2 = strtok(NULL, " ");
        char *unzip_flag = strtok(NULL, " ");
        if (date1 == NULL || date2 == NULL) {
            return 0;
        }
        if (!is_valid_date_format(date1) || !is_valid_date_format(date2)) {
            return 0;
        }
        if (unzip_flag != NULL && strcmp(unzip_flag, "-u") != 0) {
            return 0;
        }
    } else if (strcmp(command_type, "quit") == 0) {
        // Check syntax for 'quit' command: quit
        // The command must be exactly 'quit'.
        if (arguments != NULL) {
            return 0;
        }
    } else {
        // Invalid command type
        return 0;
    }

    // All syntax checks passed, the command is valid
    return 1;
}

void send_tar_file(char *tar_name, int client_socket){
    // Open the generated TAR file for reading
    int tar_fd = open(tar_name, O_RDONLY);
    if (tar_fd == -1) {
        perror("Error opening TAR file");
        exit(EXIT_FAILURE);
    }

    // Get the size of the TAR file
    off_t tar_size = lseek(tar_fd, 0, SEEK_END);
    lseek(tar_fd, 0, SEEK_SET);

    // Send the TAR file contents to the client using sendfile()
    ssize_t bytes_sent = sendfile(client_socket, tar_fd, NULL, tar_size);
    if (bytes_sent == -1) {
        perror("Error sending TAR file");
        exit(EXIT_FAILURE);
    }

    // Close the TAR file
    close(tar_fd);

}

// -------------------------- handle_fgets_command ------------------------

// Function to add a file to a tar archive
int add_file_to_tar(const char *tar_filename, const char *file_path) {
    char command[256];
    snprintf(command, sizeof(command), "tar --append --file=%s %s", tar_filename, file_path);
    system(command);
}

void search_and_add_file(const char *current_directory, const char *target_file,
                         const char *tar_name) {

    DIR *dir = opendir(current_directory);
    if (dir == NULL) {
        fprintf(stderr, "Unable to open directory '%s': %s\n", current_directory, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char file_path[PATH_MAX];
        snprintf(file_path, sizeof(file_path), "%s/%s", current_directory, entry->d_name);

        struct stat file_stat;
        if (stat(file_path, &file_stat) != 0) {
            fprintf(stderr, "File '%s' not found: %s\n", file_path, strerror(errno));
            continue;
        }

        if (S_ISREG(file_stat.st_mode) && strcmp(entry->d_name, target_file) == 0) {
            // Add the file to the tar archive
            add_file_to_tar(tar_name, file_path);
        } else if (S_ISDIR(file_stat.st_mode)) {
            // Recursively search in subdirectory
            search_and_add_file(file_path, target_file, tar_name);
        }
    }

    closedir(dir);
}

void search_files(const char *files[], int num_files, const char *tar_name) {
    const char *root_directory = getenv("HOME");
    strcat(root_directory,"/");
    for (int i = 0; i < num_files; i++) {
        search_and_add_file(root_directory, files[i], tar_name);
    }
}

void handle_fgets_command(char *arguments, char *response, pid_t pro_id, int client_socket) {
    // Tokenize the space-separated file names from the arguments
    char *file_name = strtok(arguments, " ");
    char *files[4]; // Assuming the maximum of 4 files in fgets command
    int num_files = 0;

    while (file_name != NULL && num_files < 4) {
        printf("%s\n", file_name);
        files[num_files] = file_name;
        num_files++;
        file_name = strtok(NULL, " ");
    }

    if (num_files == 0) {
        // No files specified in the command
        sprintf(response, "No files specified");
    } else {
        // Create the directory if it doesn't exist
        char dir_name[PATH_MAX];
        snprintf(dir_name, sizeof(dir_name), "%d", pro_id);
        if (mkdir(dir_name, 0777) != 0 && errno != EEXIST) {
            perror("Error creating directory");
            exit(EXIT_FAILURE);
        }

        // Create the tar archive within the directory
        char tar_name[PATH_MAX];
        snprintf(tar_name, sizeof(tar_name), "%s/%s", dir_name, "temp.tar.gz");
        remove(tar_name);
        search_files(files, num_files, tar_name); // Uncomment and implement this function
        sprintf(response, "Tar archive created: %s", tar_name);
        // send_tar_file(tar_name, client_socket);
    }
}

// ----------------------------handle_tarfgetz_command------------------------------------

void handle_tarfgetz_command(char *arguments, char *response) {
    // Tokenize the space-separated arguments
    char *size1_str = strtok(arguments, " ");
    char *size2_str = strtok(NULL, " ");
    char *unzip_flag = strtok(NULL, " ");
    
    if (size1_str == NULL || size2_str == NULL) {
        sprintf(response, "Invalid arguments");
        return;
    }
    
    // Convert size1 and size2 to integers
    int size1 = atoi(size1_str);
    int size2 = atoi(size2_str);

    if (size1 < 0 || size2 < 0 || size1 > size2) {
        sprintf(response, "Invalid size criteria");
        return;
    }

    // Get the user's home directory
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        sprintf(response, "Unable to get home directory");
        return;
    }

    // Create a temporary TAR file name
    char tar_name[] = "temp.tar.gz";
    
    // Create a TAR command to find and archive files within the specified size range
    char find_command[256];
    snprintf(find_command, sizeof(find_command), "find %s -type f -size +%dB -a -size -%dB -print0 | xargs -0 tar czf %s", home_dir, size2, size1, tar_name);

    // Execute the TAR command
    if (system(find_command) != 0) {
        sprintf(response, "Error creating TAR archive");
        return;
    }

    // Check if the -u flag is specified for unzipping
    if (unzip_flag != NULL && strcmp(unzip_flag, "-u") == 0) {
        // Send the TAR archive to the client for unzipping
        // send_tar_file(tar_name, client_socket);
    }

    sprintf(response, "Tar archive created: %s", tar_name);
}


void handle_filesrch_command(char *arguments, char *response) {
    // Implement logic for handling 'filesrch' command
    // For demonstration purposes, let's assume we found the file and have the file details.
    char file_details[] = "sample.txt 1234 2023-08-06";
    strcpy(response, file_details);
}

void handle_targzf_command(char *arguments, char *response) {
    // Implement logic for handling 'targzf' command
    // For demonstration purposes, let's assume we found the files and create the tar archive.
    char tar_name[] = "temp.tar.gz";
    sprintf(response, "Tar archive created: %s", tar_name);
}

void handle_getdirf_command(char *arguments, char *response) {
    // Implement logic for handling 'getdirf' command
    // For demonstration purposes, let's assume we found the files and create the tar archive.
    char tar_name[] = "temp.tar.gz";
    sprintf(response, "Tar archive created: %s", tar_name);
}

void processclient(int client_socket, pid_t pro_id) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    printf("%d\n", client_socket);
    while (1) {
        // Receive command from the client
        bytes_received = read(client_socket, buffer, 1024);
        // bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            // Client disconnected or error occurred
            break;
        }

        buffer[bytes_received] = '\0';

        // // Validate the received command syntax
        // if (!validate_command(buffer)) {
        //     char response[] = "Invalid command syntax";
        //     send(client_socket, response, strlen(response), 0);
        //     continue;
        // }

        // Process the client command and send appropriate responses
        char response[BUFFER_SIZE] = {0};

        // Tokenize the command to extract the command type and arguments
        char *command_type = strtok(buffer, " ");
        char *arguments = strtok(NULL, "");

        if (strcmp(command_type, "fgets") == 0) {
            handle_fgets_command(arguments, response, pro_id, client_socket);
        } else if (strcmp(command_type, "tarfgetz") == 0) {
            handle_tarfgetz_command(arguments, response);
        } else if (strcmp(command_type, "filesrch") == 0) {
            handle_filesrch_command(arguments, response);
        } else if (strcmp(command_type, "targzf") == 0) {
            handle_targzf_command(arguments, response);
        } else if (strcmp(command_type, "getdirf") == 0) {
            handle_getdirf_command(arguments, response);
        } else if (strcmp(command_type, "quit") == 0) {
            // Handle 'quit' command
            char quit_response[] = "Goodbye!";
            send(client_socket, quit_response, strlen(quit_response), 0);
            break;
        } else {
            // Invalid command
            char invalid_response[] = "Invalid command";
            send(client_socket, invalid_response, strlen(invalid_response), 0);
        }

        // Send the response back to the client
        send(client_socket, response, strlen(response), 0);
    }
}


int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    int listen_fd, connection_fd, portNumber;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pid_t child_pid;

    
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Could not create socket\n");
        exit(1);
    }

    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((uint16_t) PORT);

    
    // Bind socket to address and port
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        unlink(server_socket);
        close(server_socket);
        exit(1);
    }

    // Listen for client connections
    if (listen(server_socket, 5) < 0) {
        perror("Error listening");
        close(server_socket);
        exit(1);
    }

    while (1) {
        // Accept client connection
        client_socket = accept(server_socket, (struct sockaddr *) NULL, NULL);
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
            pid_t  pro_id = getpid();
            processclient(client_socket, pro_id);
        } else {
            // Parent process
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}