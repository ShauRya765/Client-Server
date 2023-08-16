//Section# 2 | 110094989 | Shaurya Sharma

//Section# 2 | 110096129 | Harshil Hitendrabhai Panchal
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
#include <time.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT 9002
#define BUFFER_SIZE 2048
#define PATH_MAX 10000
#define FILE_TRANSFER_PORT 9003

void process_client(int client_socket, pid_t pro_id);

// -------------------------- process_fgets_command ------------------------

// Function to add a file to a tar archive
void add_file_to_tar(const char *tar_filename, const char *file_path, int *flag, char *response) {
    char command[256];
    snprintf(command, sizeof(command), "tar --append --file=%s %s", tar_filename, file_path);
    if (system(command) != 0) {
        sprintf(response, "Error creating TAR archive");
        printf("file not found");
        *flag = 0;
        return;
    }
}

void search_and_add_file(const char *current_directory, const char *target_file,
                         const char *tar_name, int *flag, char *response) {

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
            printf("not found");
            *flag = 0;
            continue;
        }

        if (S_ISREG(file_stat.st_mode) && strcmp(entry->d_name, target_file) == 0) {
            // Add the file to the tar archive
            add_file_to_tar(tar_name, file_path, flag, response);
        } else if (S_ISDIR(file_stat.st_mode)) {
            // Recursively search in subdirectory
            search_and_add_file(file_path, target_file, tar_name, flag, response);
        }
    }

    closedir(dir);
}

void search_files(const char *files[], int num_files, const char *tar_name, int *flag, char *response) {
    const char *root_directory = getenv("HOME");
    strcat(root_directory, "/");
    for (int i = 0; i < num_files; i++) {
        search_and_add_file(root_directory, files[i], tar_name, flag, response);
    }
}

void send_tar_file(const char *file_path, int socket) {
    char buffer[2048];
    FILE *fp = fopen(file_path, "r");
    if (fp == NULL) {
        perror("Cannot open file to send");
        return;
    }

    // Get the size of the file.
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);


    // Send file size.
    if (send(socket, &file_size, sizeof(file_size), 0) < 0) {
        perror("Send file size failed");
        fclose(fp);
        return;
    }

    while (!feof(fp)) {
        size_t read_size = fread(buffer, 1, sizeof(buffer), fp);
        if (read_size > 0) {
            if (send(socket, buffer, read_size, 0) < 0) {
                perror("Send failed");
                break;
            }
        }
    }

    fclose(fp);
    memset(buffer, 0, sizeof(buffer));

}


void process_fgets_command(char *arguments, char *response, pid_t pro_id, int client_socket) {
    // Tokenize the space-separated file names from the arguments
    char *file_name = strtok(arguments, " ");
    char *files[4]; // Assuming the maximum of 4 files in fgets command
    int num_files = 0;
    int start_flag = 1;

    while (file_name != NULL && num_files < 4) {
        files[num_files] = file_name;
        num_files++;
        file_name = strtok(NULL, " ");
    }

    if (num_files == 0) {
        // No files specified in the command
        sprintf(response, "No files specified");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    } else {
        // Create the directory if it doesn't exist
        char dir_name[PATH_MAX];
        snprintf(dir_name, sizeof(dir_name), "%d", pro_id);
        if (mkdir(dir_name, 0777) != 0 && errno != EEXIST) {
            perror("Error creating directory");
            sprintf(response, "Error creating directory");
            start_flag = 0;
            send(client_socket, &start_flag, sizeof(int), 0);
            return;
        }

        // Create the tar archive within the directory
        char tar_name[PATH_MAX];
        snprintf(tar_name, sizeof(tar_name), "%s/%s", dir_name, "temp.tar.gz");
        remove(tar_name); // previous temp tar file deleting
        search_files(files, num_files, tar_name, &start_flag, response); // Uncomment and implement this function

        // Check if the file exists
        if (access(tar_name, F_OK) != -1) {
            start_flag = 1;
        } else {
            start_flag = 0;
        }

        if (start_flag == 1) {
            int flag = 1;
            sprintf(response, "Tar archive created: %s", tar_name);
            send(client_socket, &flag, sizeof(int), 0);
            send_tar_file(tar_name, client_socket);
            send(client_socket, response, strlen(response), 0);
        } else {
            send(client_socket, &start_flag, sizeof(int), 0);
            sprintf(response, "No file found");
        }


    }
}

// ----------------------------process_tarfgetz_command------------------------------------

void process_tarfgetz_command(char *arguments, char *response, pid_t pro_id, int client_socket) {
    // Tokenize the space-separated arguments
    char *size1_str = strtok(arguments, " ");
    char *size2_str = strtok(NULL, " ");
    char *unzip_flag = strtok(NULL, " ");
    int start_flag = 1;

    if (size1_str == NULL || size2_str == NULL) {
        sprintf(response, "Invalid arguments");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Convert size1 and size2 to integers
    int size1 = atoi(size1_str);
    int size2 = atoi(size2_str);

    if (size1 < 0 || size2 < 0 || size1 > size2) {
        sprintf(response, "Invalid size criteria");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Get the user's home directory
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        sprintf(response, "Unable to get home directory");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Create a temporary TAR file name
    char tar_name[PATH_MAX];


    // Create the directory if it doesn't exist
    char dir_name[PATH_MAX];
    snprintf(dir_name, sizeof(dir_name), "%d", pro_id);
    if (mkdir(dir_name, 0777) != 0 && errno != EEXIST) {
        sprintf(response, "Error creating directory");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Use the find command to write the list of files to the file
    char find_command[256];
    snprintf(find_command, sizeof(find_command), "find ~ -type f -size +%dk -a -size -%dk > %d/file_list.txt", size1,
             size2, pro_id);
    if (system(find_command) != 0) {
        sprintf(response, "No files found");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    char filename[256];
    snprintf(filename, sizeof(filename), "%d/file_list.txt", pro_id);
    // Open the file in binary read mode
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Seek to the end of the file
    fseek(file, 0, SEEK_END);

    // Get the current position (which is the size of the file)
    long file_size = ftell(file);

    if (file_size == 0) {
        sprintf(response, "No files found");
        start_flag = 0;
        printf("not found");
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Close the file
    fclose(file);


    // Create the TAR archive using the file list
    char tar_command[256];

    snprintf(tar_name, sizeof(tar_name), "%s/%s", dir_name, "temp.tar.gz");
    remove(tar_name); // previous temp tar file deleting

    snprintf(tar_command, sizeof(tar_command), "tar czf %s -T %d/file_list.txt", tar_name, pro_id);
    if (system(tar_command) != 0) {
        sprintf(response, "Error creating TAR archive");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Check if the -u flag is specified for unzipping
    if (unzip_flag != NULL && strcmp(unzip_flag, "-u") == 0) {
        // Send the TAR archive to the client for unzipping
        // send_tar_file(tar_name, client_socket);
    }

    if (start_flag == 1) {
        sprintf(response, "Tar archive created: %s", tar_name);
        send(client_socket, &start_flag, sizeof(int), 0);
        send_tar_file(tar_name, client_socket);
        return;
    } else {
        sprintf(response, "No files found");
        send(client_socket, &start_flag, sizeof(int), 0);
    }
}

// ---------------------------------process_filesrch_command---------------------------------

void format_creation_time(time_t ctime, char *formatted_time) {
    struct tm *timeinfo;
    timeinfo = localtime(&ctime);
    strftime(formatted_time, 20, "%b %d %H:%M", timeinfo);
}

void process_filesrch_command(char *arguments, char *response, int client_socket) {
    // Tokenize the command arguments to get the filename
    int start_flag = 0;
    char *filename = strtok(arguments, " ");
    if (filename == NULL) {
        sprintf(response, "No filename specified");
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Get the user's home directory
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        sprintf(response, "Unable to get home directory");
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Create a buffer to store the full path to the target file
    char target_path[PATH_MAX];

    // Use the find command to search for the file
    char find_command[256];
    snprintf(find_command, sizeof(find_command), "find %s -type f -name %s | head -n 1", home_dir, filename);

    FILE *find_output = popen(find_command, "r");
    if (find_output == NULL) {
        sprintf(response, "Error searching for file");
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Read the first line of the find command output, which should be the path to the file
    if (fgets(target_path, sizeof(target_path), find_output) == NULL) {
        pclose(find_output);
        sprintf(response, "File not found");
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Remove the newline character from the path
    target_path[strlen(target_path) - 1] = '\0';

    // Close the find command output pipe
    pclose(find_output);



    // Get file size and creation time
    struct stat file_stat;
    if (stat(target_path, &file_stat) != 0) {
        send(client_socket, &start_flag, sizeof(int), 0);
        sprintf(response, "Error getting file information");
        return;
    }

    // Convert the st_ctime value to formatted creation time
    char formatted_time[20];
    format_creation_time(file_stat.st_ctime, formatted_time);

    // Format the response with filename, size, and formatted creation time
    sprintf(response, "%s %lld %s", filename, (long long) file_stat.st_size, formatted_time);
    send(client_socket, &start_flag, sizeof(int), 0);
}

// -------------------------------process_targzf_command---------------------------------------------

void process_targzf_command(char *arguments, char *response, pid_t pro_id, int client_socket) {
    // Tokenize the space-separated arguments
    char *extension_list = strtok(arguments, " ");
    int start_flag = 1;

    // Create the directory if it doesn't exist
    char dir_name[PATH_MAX];
    snprintf(dir_name, sizeof(dir_name), "%d", pro_id);
    if (mkdir(dir_name, 0777) != 0 && errno != EEXIST) {
        perror("Error creating directory");
        sprintf(response, "Error creating directory");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    if (extension_list == NULL) {
        sprintf(response, "Invalid arguments");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Extract file extensions from the extension list
    char *extensions[6]; // Assuming maximum of 6 extensions
    int num_extensions = 0;

    while (extension_list != NULL && num_extensions < 6) {
        extensions[num_extensions] = extension_list;
        num_extensions++;
        extension_list = strtok(NULL, " ");
    }

    char *unzip_flag = strtok(NULL, " ");

    if (num_extensions == 0) {
        // No extensions specified in the command
        sprintf(response, "No file extensions specified");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    char file_list_path[PATH_MAX];
    snprintf(file_list_path, sizeof(file_list_path), "%s/%s", dir_name, "file_list.txt");
    // Create a temporary TAR file name
    char tar_name[PATH_MAX];
    snprintf(tar_name, sizeof(tar_name), "%s/temp.tar.gz", dir_name);

    // Create a temporary file to store the list of files to archive
    FILE *file_list = fopen(file_list_path, "w");
    if (file_list == NULL) {
        sprintf(response, "Error creating file list");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }


    // Get the user's home directory
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        sprintf(response, "Unable to get home directory");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        fclose(file_list);
        return;
    }

    // Use the find command to search for files with specified extensions
    char find_command[256];
    snprintf(find_command, sizeof(find_command), "find %s -type f ", home_dir);

    for (int i = 0; i < num_extensions; i++) {
        if (i > 0) {
            strcat(find_command, "-o ");
        }
        strcat(find_command, "-name '*.");
        strcat(find_command, extensions[i]);
        strcat(find_command, "' ");
    }


    strcat(find_command, " > ");
    strcat(find_command, file_list_path);
    // Execute the find command to generate the file list

    if (system(find_command) != 0) {
        sprintf(response, "No files found");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Close the file
    fclose(file_list);

    // Create the TAR archive using the file list
    char tar_command[PATH_MAX + 50];
    snprintf(tar_command, sizeof(tar_command), "tar czf %s -T %s", tar_name, file_list_path);
    if (system(tar_command) != 0) {
        sprintf(response, "Error creating TAR archive");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Check if the -u flag is specified for unzipping
    if (unzip_flag != NULL && strcmp(unzip_flag, "-u") == 0) {
        // Send the TAR archive to the client for unzipping
        // send_tar_file(tar_name, client_socket);
    }

    if (start_flag == 1) {
        sprintf(response, "Tar archive created: %s", tar_name);
        send(client_socket, &start_flag, sizeof(int), 0);
        send_tar_file(tar_name, client_socket);
    } else {
        sprintf(response, "No file found");
        send(client_socket, &start_flag, sizeof(int), 0);
    }
}


void process_getdirf_command(char *arguments, char *response, int client_socket) {
    // Tokenize the command arguments
    char *date1 = strtok(arguments, " ");
    char *date2 = strtok(NULL, " ");
    char *unzip_flag = strtok(NULL, " ");
    int start_flag = 1;

    if (date1 == NULL || date2 == NULL) {
        sprintf(response, "Invalid arguments");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Get the user's home directory
    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        sprintf(response, "Unable to get home directory");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Create a directory to store the TAR archive
    char dir_name[PATH_MAX];
    snprintf(dir_name, sizeof(dir_name), "%d", (int) getpid());
    if (mkdir(dir_name, 0777) != 0 && errno != EEXIST) {
        perror("Error creating directory");
        sprintf(response, "Error creating directory");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Create a temporary TAR file name
    char tar_name[PATH_MAX];
    snprintf(tar_name, sizeof(tar_name), "%s/temp.tar.gz", dir_name);

    // Use the find command to search for files within the specified date range
    char find_command[BUFFER_SIZE];
    snprintf(find_command, sizeof(find_command), "find %s -type f -newermt %s ! -newermt %s > %s/file_list.txt",
             home_dir, date1, date2, dir_name);

    if (system(find_command) != 0) {
        sprintf(response, "Error creating TAR archive");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Create the TAR archive using the file list
    char tar_command[BUFFER_SIZE];
    snprintf(tar_command, sizeof(tar_command), "tar czf %s -T %s/file_list.txt", tar_name, dir_name);
    if (system(tar_command) != 0) {
        sprintf(response, "Error creating TAR archive");
        start_flag = 0;
        send(client_socket, &start_flag, sizeof(int), 0);
        return;
    }

    // Check if the -u flag is specified for unzipping
    if (unzip_flag != NULL && strcmp(unzip_flag, "-u") == 0) {
        // Send the TAR archive to the client for unzipping
        // send_tar_file(tar_name, client_socket);
    }

    if (start_flag == 1) {
        sprintf(response, "Tar archive created: %s", tar_name);
        send(client_socket, &start_flag, sizeof(int), 0);
        send_tar_file(tar_name, client_socket);
    } else {
        sprintf(response, "No file found");
        send(client_socket, &start_flag, sizeof(int), 0);
    }

}

// process client function to process command from each client in new child process
void process_client(int client_socket, pid_t pro_id) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while (1) {
        // Receive command from the client
        bytes_received = read(client_socket, buffer, 1024);
        // bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            // Client disconnected or error occurred
            break;
        }

        buffer[bytes_received] = '\0';

        // Process the client command and send appropriate responses
        char response[BUFFER_SIZE] = {0};

        // Tokenize the command to extract the command type and arguments
        char *command_type = strtok(buffer, " ");
        char *arguments = strtok(NULL, "");

        if (strcmp(command_type, "fgets") == 0) {
            process_fgets_command(arguments, response, pro_id, client_socket);
        } else if (strcmp(command_type, "tarfgetz") == 0) {
            process_tarfgetz_command(arguments, response, pro_id, client_socket);
        } else if (strcmp(command_type, "filesrch") == 0) {
            process_filesrch_command(arguments, response, client_socket);
        } else if (strcmp(command_type, "targzf") == 0) {
            process_targzf_command(arguments, response, pro_id, client_socket);
        } else if (strcmp(command_type, "getdirf") == 0) {
            process_getdirf_command(arguments, response, client_socket);
        } else if (strcmp(command_type, "quit") == 0) {
            // Handle 'quit' command
            char quit_response[] = "Goodbye!";
            send(client_socket, quit_response, strlen(quit_response), 0);
            kill(pro_id, 0);
            break;
        } else {
            // Invalid command
            char invalid_response[] = "Invalid command";
            send(client_socket, invalid_response, strlen(invalid_response), 0);
        }

        // Send the response back to the client
        printf("%s", response);
        send(client_socket, response, strlen(response), 0);
        memset(response, 0, sizeof(response));
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
    server_addr.sin_port = htons((uint16_t) atoi(argv[1]));


    // Bind socket to address and port
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
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
            pid_t pid = getpid();
            process_client(client_socket, pid);
        } else {
            // Parent process
            close(client_socket);
        }
    }
}