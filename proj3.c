// Name: Shibo Wang
// Case ID: sxw1127
// Filename: proj3.c
// Date Created: 10/18/2023
// Description: the main function of the proj3
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "socket_methods.h"

int main(int argc, char *argv[]) {
    struct sockaddr_in sin;
    struct sockaddr addr;
    struct protoent *protoinfo;
    struct timeval timeout;
    struct Opts opts;
    unsigned int addrlen;
    int sd, sd2;

    if (argc != REQUIRED_ARGC)
        usage(argv[PROJECT_POSITION]);

    // set time out
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_USEC;

    // parse the arguments to opts
    parseargs(&opts, argc, argv);

    // determine protocol
    if ((protoinfo = getprotobyname(PROTOCOL)) == NULL)
        errexit("cannot find protocol information for %s", PROTOCOL);

    // setup endpoint info
    memset((char *) &sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons((u_short) atoi(opts.port));

    // allocate a socket
    // would be SOCK_DGRAM for UDP
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < ERROR_BOUND)
        errexit("cannot create socket", NULL);

    // bind the socket
    if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) < ERROR_BOUND)
        errexit("cannot bind to port %s", opts.port);

    // listen for incoming connections
    if (listen(sd, QLEN) < ERROR_BOUND)
        errexit("cannot listen on port %s\n", opts.port);

    // accept a connection
    addrlen = sizeof(addr);
    while (!opts.shutdown) {
        // build socket2
        sd2 = accept(sd, &addr, &addrlen);
        if (sd2 < ERROR_BOUND)
            errexit("error accepting connection", NULL);

        // set timeout
        if (setsockopt(sd2, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < ERROR_BOUND) {
            errexit("failed to set up timeout", NULL);
        }

        // decode socket2
        char *request = socket_decode(sd2);

        // write message to the connection
        get_response(sd2, &opts, request);

        // close socket2
        close(sd2);
        free(request);
    }

    // close connections and exit
    cleanupStruct(&opts);
    close(sd);
    exit(0);
}