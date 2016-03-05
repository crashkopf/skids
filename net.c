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

void pack (unsigned char *b, uint16_t c, int32_t v) {
	c = htons(c);
	memcpy(&b[PKT_CMD], &c, sizeof(c));
	v = htonl(v);
	memcpy(&b[PKT_VAL], &v, sizeof(v));
}

void unpack (unsigned char *b, uint16_t *c, int32_t *v) {
	memcpy(c, &b[PKT_CMD], sizeof(uint16_t));
	*c = ntohs(*c);
	memcpy(v, &b[PKT_VAL], sizeof(int32_t));
	*v = ntohl(*v);
}