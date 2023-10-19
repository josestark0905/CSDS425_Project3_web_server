// Name: Shibo Wang
// Case ID: sxw1127
// Filename: socket_methods.c
// Date Created: 10/18/2023
// Description: source file of all implemented functions
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <regex.h>
#include <wordexp.h>
#include "socket_methods.h"

/* if the input arguments is INVALID, show the way to specify each option and the function of each option */
void usage(char *program_name) {
    fprintf(stderr, "%s -n <port> -d <document_directory> -a <auth_token>\n", program_name);
    fprintf(stderr, "   -n <port>                 MUST CONTAIN: set the port program listens to\n");
    fprintf(stderr, "   -d <document_directory>   MUST CONTAIN: set the directory of source files\n");
    fprintf(stderr, "   -a <auth_token>           MUST CONTAIN: set the auth_token for shutdown method\n");
    exit(ERROR);
}

/* when the host name is INVALID, throw an error to show the host cannot be found */
void errexit(char *format, char *arg) {
    fprintf(stderr, format, arg);
    fprintf(stderr, "\n");
    exit(ERROR);
}

/* check whether the protocol is exactly "HTTP/" */
bool check_protocol(char *Protocol) {
    if (strncmp(Protocol, "HTTP/", HTTP_PROTOCOL_LEN) == HTTP_PROTOCOL_ACCEPT) {
        return true;
    } else {
        return false;
    }
}

/* extract the opts into a structure */
void parseargs(struct Opts *opts, int argc, char *argv[]) {
    int opt;
    int index = INITIAL_ARGV;
    char *Port, *Document_directory, *Auth_token;
    bool ALL_valid = true;
    //initialize the opts
    opts->n_flag = false;
    opts->d_flag = false;
    opts->a_flag = false;
    opts->shutdown = false;
    opts->port = NULL;
    opts->document_directory = NULL;
    opts->auth_token = NULL;
    opts->argument = NULL;

    if (argc <= MIN_ARGC) {
        fprintf(stderr, "no enough arguments");
        usage(argv[PROJECT_POSITION]);
    } else {
        while (index < argc) {
            if (strlen(argv[index]) == 2) {
                if (argv[index][0] == '-') {
                    if (argv[index][1] == 'n' || argv[index][1] == 'd' || argv[index][1] == 'a') {
                        index++;
                    }
                } else {
                    fprintf(stderr, "invalid parameter %s\n", argv[index]);
                    ALL_valid = false;
                }
            } else {
                fprintf(stderr, "invalid parameter %s\n", argv[index]);
                ALL_valid = false;
            }
            index++;
        }
    }
    while ((opt = getopt(argc, argv, "n:d:a:")) != OPTION_END) {
        switch (opt) {
            case 'n':
                opts->n_flag = true;
                Port = optarg;
                break;
            case 'd':
                opts->d_flag = true;
                Document_directory = optarg;
                break;
            case 'a':
                opts->a_flag = true;
                Auth_token = optarg;
                break;
            case '?':
                ALL_valid = false;
                if (optopt == 'n' || optopt == 'd' || optopt == 'a') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option -%c\n", optopt);
                }
                break;
            default:
                usage(argv[PROJECT_POSITION]);
        }
    }
    if (!ALL_valid || !opts->n_flag || !opts->d_flag || !opts->a_flag) {
        usage(argv[PROJECT_POSITION]);
    } else {
        opts->port = strdup(Port);
        wordexp_t Doc_Dir;
        if (wordexp(Document_directory, &Doc_Dir, 0) != 0) {
            errexit("word expansion of document directory failed", NULL);
        }
        opts->document_directory = strdup(Doc_Dir.we_wordv[0]);
        wordfree(&Doc_Dir);
        opts->auth_token = strdup(Auth_token);
    }
}

/* read all the information from socket, extend the buffer length if necessary. Process the content in data then */
char *socket_decode(int socket) {
    ssize_t ret;
    size_t buffer_size = BUFLEN;
    size_t bytes_read = INITIAL_BYTE;
    // Dynamically allocate the memory of Buffer, the initial memory size is 1024
    char *Buffer = (char *) malloc(BUFLEN);
    // Initialize the allocated memory
    memset(Buffer, 0x0, BUFLEN);
    while (true) {
        ret = recv(socket, Buffer + bytes_read, buffer_size - bytes_read - 1, 0);
        if (ret > ERROR_READ)
            Buffer[bytes_read + ret] = '\0';
        if (ret == ERROR_READ || strstr(Buffer, "\n\r\n")) {
            break;
        } else {
            bytes_read += ret;
            // Check whether the Buffer needs to be extended
            if (bytes_read == buffer_size - END_POSITION) {
                buffer_size += BUF_EXTEND;
                char *new_buffer = (char *) realloc(Buffer, buffer_size);
                if (new_buffer == NULL) {
                    free(Buffer);
                    fprintf(stderr, "Memory reallocation failed\n");
                    exit(ERROR);
                }
                Buffer = new_buffer;
                // Initialize the reallocated memory
                memset(Buffer + bytes_read, 0x0, buffer_size - bytes_read);
            }
        }
    }
    return Buffer;
}

/* pre-process of the request */
int check_request(struct Opts *opts, const char *request) {
    const char *pattern = "^([^ \r\n]+) ([^ \r\n]+) ([^ \r\n]+)\r\n(([^\r\n]+\r\n)*)\r\n$";
    int status;
    regex_t regex;
    regmatch_t matches[REG_CHECK]; // 4 groups + whole expression

    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        errexit("Failed to compile regex.\n", NULL);
    }

    if (regexec(&regex, request, REG_CHECK, matches, 0) == 0) {
        // get the matched parts
        int length_method = matches[1].rm_eo - matches[1].rm_so;
        int length_path = matches[2].rm_eo - matches[2].rm_so;
        int length_protocol = matches[3].rm_eo - matches[3].rm_so;
        char method[length_method + END_POSITION], argument[length_path + END_POSITION], protocol[
                length_protocol + END_POSITION];

        // copy METHOD
        strncpy(method, &request[matches[1].rm_so], length_method);
        method[length_method] = '\0';

        // copy PATH
        strncpy(argument, &request[matches[2].rm_so], length_path);
        argument[length_path] = '\0';

        // copy PROTOCOL
        strncpy(protocol, &request[matches[3].rm_so], length_protocol);
        protocol[length_protocol] = '\0';

        // start other check
        if (!check_protocol(protocol)) {
            status = PROTOCOL_NOT_IMPLEMENTED;
        } else {
            if (strcmp(method, "GET") == 0) {
                if (strcmp(argument, "/") == 0) {
                    opts->argument = strdup("/homepage.html");
                } else {
                    opts->argument = strdup(argument);
                }
                status = CALL_GET;
            } else if (strcmp(method, "SHUTDOWN") == 0) {
                opts->argument = strdup(argument);
                status = CALL_SHUTDOWN;
            } else {
                status = UNSUPPORTED_METHOD;
            }
        }
    } else {
        status = MALFORMED_REQUEST;
    }
    regfree(&regex);
    return status;
}

/* according to the pre-process of request, write the actual response to socket */
void get_response(int socket, struct Opts *opts, char *request) {
    char *response = NULL;
    int status = check_request(opts, request);
    if (status == MALFORMED_REQUEST) {
        response = "HTTP/1.1 400 Malformed Request\r\n\r\n";
    }
    if (status == PROTOCOL_NOT_IMPLEMENTED) {
        response = "HTTP/1.1 501 Protocol Not Implemented\r\n\r\n";
    }
    if (status == UNSUPPORTED_METHOD) {
        response = "HTTP/1.1 405 Unsupported Method\r\n\r\n";
    }
    if (status == CALL_GET) {
        if (opts->argument[0] != '/') {
            status = INVALID_FILENAME;
            response = "HTTP/1.1 406 Invalid Filename\r\n\r\n";
        } else {
            // Determine combined length
            size_t combinedLength = strlen(opts->document_directory) + strlen(opts->argument) + 1;

            // Allocate memory for the concatenated string
            char *dir = (char *) malloc(combinedLength);
            if (dir == NULL) {
                errexit("Memory allocation failed", NULL);
            }

            // Concatenate the strings
            strcpy(dir, opts->document_directory);
            strcat(dir, opts->argument);

            // Open the file
            FILE *file = fopen(dir, "rb");
            if (file == NULL) {
                status = FILE_NOT_FOUND;
                response = "HTTP/1.1 404 File Not Found\r\n\r\n";
            } else {
                response = "HTTP/1.1 200 OK\r\n\r\n";
                if (write(socket, response, strlen(response)) < 0)
                    errexit("error writing message: %s", response);
                char buffer[64];
                size_t bytes_read;
                // Read file and send its contents over the socket
                while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                    send(socket, buffer, bytes_read, 0);
                }
                fclose(file);
            }
            free(dir);
        }
    }
    if (status == CALL_SHUTDOWN) {
        if (strcmp(opts->argument, opts->auth_token) == 0) {
            opts->shutdown = true;
            response = "HTTP/1.1 200 Server Shutting Down\r\n\r\n";
        } else {
            status = OPERATION_FORBIDDEN;
            response = "HTTP/1.1 403 Operation Forbidden\r\n\r\n";
        }
    }
    if (status != CALL_GET) {
        if (write(socket, response, strlen(response)) < 0)
            errexit("error writing message: %s", response);
    }
    if (opts->argument != NULL) {
        free(opts->argument);
        opts->argument = NULL;
    }
}

/* free all the allocated memory in structure */
void cleanupStruct(struct Opts *opts) {
    if (opts->port != NULL) {
        free(opts->port);
        opts->port = NULL;
    }
    if (opts->document_directory != NULL) {
        free(opts->document_directory);
        opts->document_directory = NULL;
    }
    if (opts->auth_token != NULL) {
        free(opts->auth_token);
        opts->auth_token = NULL;
    }
}
