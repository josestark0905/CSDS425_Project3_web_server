// Name: Shibo Wang
// Case ID: sxw1127
// Filename: socket_methods.h
// Date Created: 10/18/2023
// Description: header file of all declared functions and macro definitions
#ifndef PROJECT3_SOCKET_METHODS_H
#define PROJECT3_SOCKET_METHODS_H

#define REQUIRED_ARGC 7
#define ERROR 1
#define QLEN 1
#define PROTOCOL "tcp"
#define BUFLEN 8
#define BUF_EXTEND 8
#define INITIAL_ARGV 1
#define MIN_ARGC 1
#define PROJECT_POSITION 0
#define OPTION_END (-1)
#define INITIAL_BYTE 0
#define ERROR_READ (-1)
#define COMPLETE_READ 0
#define ERROR_BOUND 0
#define END_POSITION 1
#define TIMEOUT_SEC 0
#define HTTP_PROTOCOL_LEN 5
#define HTTP_PROTOCOL_ACCEPT 0
#define REG_CHECK 5
#define MALFORMED_REQUEST 400
#define PROTOCOL_NOT_IMPLEMENTED 501
#define CALL_GET 201
#define CALL_SHUTDOWN 202
#define UNSUPPORTED_METHOD 405
#define INVALID_FILENAME 406
#define FILE_NOT_FOUND 404
#define OPERATION_FORBIDDEN 403
#define TIMEOUT_USEC 500000

#include <stdbool.h>

/*Structure containing the options get from command line.*/
struct Opts {
    bool n_flag;
    bool d_flag;
    bool a_flag;
    bool shutdown;
    char *port;
    char *document_directory;
    char *auth_token;
    char *argument;
};

void usage(char *progname);

void errexit(char *format, char *arg);

bool check_protocol(char* Protocol);

void parseargs(struct Opts *opts, int argc, char *argv[]);

char* socket_decode(int socket);

int check_request(struct Opts *opts, const char* request);

void get_response(int sd2, struct Opts *opts, char* request);

void cleanupStruct(struct Opts *opts);

#endif //PROJECT3_SOCKET_METHODS_H
