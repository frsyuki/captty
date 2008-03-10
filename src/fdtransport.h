/*
 * This file is part of Partty
 *
 * Copyright (C) 2008-2009 FURUHASHI Sadayuki
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FD_TRANSPORT_H__
#define FD_TRANSPORT_H__

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>

static inline int listen_gate(const char* path)
{
	int gate;
	struct sockaddr_un gate_addr;
	if( (gate = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0 ) {
		return -1;
	}
	memset(&gate_addr, 0, sizeof(gate_addr));
	gate_addr.sun_family = AF_UNIX;
	strncpy(gate_addr.sun_path, path, sizeof(gate_addr.sun_path)-1);
	gate_addr.sun_path[sizeof(gate_addr.sun_path)-1] = '\0';
	//strlcpy(gate_addr.sun_path, path, sizeof(gate_addr.sun_path));
	if( bind(gate, (struct sockaddr*)&gate_addr, sizeof(gate_addr)) < 0 ) {
		return -1;
	}
	return gate;
}

static inline int connect_gate(const char* path)
{
	int gate;
	struct sockaddr_un gate_addr;
	if ( (gate = socket(PF_UNIX, SOCK_DGRAM, 0)) < 0 ) {
		return -1;
	}
	memset(&gate_addr, 0, sizeof(gate_addr));
	gate_addr.sun_family = AF_UNIX;
	strncpy(gate_addr.sun_path, path, sizeof(gate_addr.sun_path)-1);
	gate_addr.sun_path[sizeof(gate_addr.sun_path)-1] = '\0';
	//strlcpy(gate_addr.sun_path, path, sizeof(gate_addr.sun_path));
	if ( connect(gate, (struct sockaddr*)&gate_addr, sizeof(gate_addr)) < 0 ) {
		return -1;
	}
	return gate;
}

static inline int recvfd(int gate, void* message, size_t message_len)
{
	struct msghdr msg;
	struct iovec iov;
	char cmsgbuf[CMSG_SPACE(sizeof(int))];

	iov.iov_base = message;
	iov.iov_len = message_len;
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = MSG_WAITALL;

	if ( recvmsg(gate, &msg, 0) < 0 ) {
		return -1;
	}

	cmsghdr *cmsg = (cmsghdr*)cmsgbuf;
	return *((int *)CMSG_DATA(cmsg));
}


static inline int sendfd(int gate, int fd, void* message, int message_len)
{
	struct iovec iov;
	char cmsgbuf[CMSG_SPACE(sizeof(int))];

	iov.iov_base = message;
	iov.iov_len = message_len;

	cmsghdr *cmsg = (cmsghdr*)cmsgbuf;
	cmsg->cmsg_len = CMSG_LEN(sizeof(int));
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	*((int *)CMSG_DATA(cmsg)) = fd;

	msghdr msg;
	memset(&msg, 0, sizeof(msg));
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;

	if (sendmsg(gate, &msg, 0) < 0) {
		return -1;
	}
	return 0;
}


#endif /* fdtransport.h */
