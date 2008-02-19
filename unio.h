#ifndef UNIO_H__
#define UNIO_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

inline void pexit(const char* msg)
{
	perror(msg);
	exit(1);
}

inline int listen_path(const char* path)
{
	int sock;
	struct sockaddr_un gate_addr;
	if( (sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0 ) {
		return -1;
	}
	memset(&gate_addr, 0, sizeof(gate_addr));
	gate_addr.sun_family = AF_UNIX;
	strncpy(gate_addr.sun_path, path, sizeof(gate_addr.sun_path)-1);
	gate_addr.sun_path[sizeof(gate_addr.sun_path)-1] = '\0';
	//strlcpy(gate_addr.sun_path, path, sizeof(gate_addr.sun_path));
	if( bind(sock, (struct sockaddr*)&gate_addr, sizeof(gate_addr)) < 0 ) {
		return -1;
	}
	if( listen(sock, 5) < 0 ) {
		return -1;
	}
	return sock;
}

inline int listen_tcp(unsigned short port)
{
	int sock;
	struct sockaddr_in addr;
	int on = 1;
	if( (sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
		return -1;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ) {
		return -1;
	}
	if( bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0 ) {
		return -1;
	}
	if( listen(sock, 5) < 0 ) {
		return -1;
	}
	return sock;
}


inline int connect_path(const char* path)
{
	int sock;
	struct sockaddr_un gate_addr;
	if ( (sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0 ) {
		return -1;
	}
	memset(&gate_addr, 0, sizeof(gate_addr));
	gate_addr.sun_family = AF_UNIX;
	strncpy(gate_addr.sun_path, path, sizeof(gate_addr.sun_path)-1);
	gate_addr.sun_path[sizeof(gate_addr.sun_path)-1] = '\0';
	//strlcpy(gate_addr.sun_path, path, sizeof(gate_addr.sun_path));
	if ( connect(sock, (struct sockaddr*)&gate_addr, sizeof(gate_addr)) < 0 ) {
		return -1;
	}
	return sock;
}


inline size_t read_all(int fd, void *buf, size_t count) {
	char *p = (char*)buf;
	char * const endp = p + count;

	do {
		const int num_bytes = read(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				return endp - p;
			}
		} else {
			p += num_bytes;
		}
	} while (p < endp);

	return count;
}

inline size_t write_all(int fd, const void *buf, size_t count) {
	const char *p = (char*)buf;
	const char * const endp = p + count;

	do {
		const int num_bytes = write(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				return endp - p;
			}
		} else {
			p += num_bytes;
		}
	} while (p < endp);

	return count;
}

#endif
