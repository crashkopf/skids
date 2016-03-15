#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "net.h"

int get_socket (char * host, char * port) {
	int status, fd;
	struct addrinfo hints, *servinfo, *p;

	// Resolve target host address
	memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((status = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(status));
        return -1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            fprintf(stderr, "Socket error: %s\n", strerror(errno));
            continue;
        }

        break;
    }
	
	freeaddrinfo(servinfo);

    if (p == NULL) {
        fprintf(stderr, "Failed to create socket\n");
        return -1;
    }
	
	if (connect(fd, p->ai_addr, p->ai_addrlen) < 0) {
		fprintf(stderr, "Connect error: %s\n", strerror(errno));
		close(fd);
		return -1;
	}
	return fd;
}
