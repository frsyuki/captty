#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>
#include "raw_gate.h"
#include "fdtransport.h"
#include "uniext.h"

#include <iostream>

namespace Partty {


RawGate::RawGate(config_t& config) : impl(new RawGateIMPL(config)) {}

RawGateIMPL::RawGateIMPL(RawGate::config_t& config) :
		socket(config.listen_socket), m_end(0)
{
	gate_dir_len = strlen(GATE_DIR);
	if( gate_dir_len > PATH_MAX ) {
		throw initialize_error("gate directory path is too long");
	}
	memcpy(gate_path, GATE_DIR, gate_dir_len);
	// この時点ではgate_pathにはNULL終端が付いていない
}


RawGate::~RawGate() { delete impl; }

RawGateIMPL::~RawGateIMPL() {}


int RawGate::run(void) { return impl->run(); }
int RawGateIMPL::run(void)
{
	pid_t pid;
	int status;
	unsigned int numchild = 0;
	while(!m_end) {
		while(numchild < 5) {
			if( (pid = fork()) < 0 ) {
				perror("failed to fork");
				throw partty_error("failed to fork");
			} else if( pid == 0 ) {
				// pid
				exit(accept_guest());
			}
			++numchild;
		}
		// parent
		pid = wait(&status);
		if( pid < 0 ) {
			perror("wait");
			break;
		}
		--numchild;
		int e = WEXITSTATUS(status);
		if( e == E_ACCEPT || e == E_FINISH ) {
			break;
		}
	}

	kill(0, SIGTERM);
	while(numchild > 0) {
		pid = wait(&status);
		if( pid < 0 ) {
			perror("wait");
			return -1;
		}
		--numchild;
	}
	perror("end");

	return 0;
}


int RawGateIMPL::accept_guest(void)
{
	int guest = accept(socket, NULL, NULL);
	if( guest  < 0 ) { return E_ACCEPT; }
	if( guest == 0 ) { return E_FINISH; }

	char buf[MAX_SESSION_NAME_LENGTH + MAX_PASSWORD_LENGTH + 2];
	char* p = buf;
	char* const pend = buf + sizeof(buf);
	ssize_t len;
	unsigned int num_zero = 0;
	while(num_zero < 2) {
		if( pend <= p ) { return E_READ; }
		len = read(guest, p, pend - p);
		if( len < 0 ) {
			if( errno == EAGAIN || errno == EINTR ) { continue; }
			else { return E_READ; }
		} else if( len == 0 ) {
			return E_READ;
		} else {
			const char* s = p;
			p += len;
			for(; s < p; ++s) {
				if(*s == '\0') { num_zero++; }
			}
		}
#ifdef PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_SUPPORT
		if( num_zero > 0 && strcmp("<policy-file-request/>", buf) == 0 ) {
			write_all(guest, PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_POLICY, strlen(PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_POLICY)+1);   // NULL終端も送る
			close(guest);
			return E_FLASH_CROSS_DOMAIN;
		}
#endif
	}

	
	gate_message_t msg;

	p = buf;
	msg.session_name.len = strlen(p);
	if( msg.session_name.len > MAX_SESSION_NAME_LENGTH ) { return E_SESSION_NAME; }
	std::memcpy(msg.session_name.str, p, msg.session_name.len);
	p += msg.session_name.len + 1;  // NULL終端分+1

	msg.password.len = strlen(p);
	if( msg.password.len > MAX_PASSWORD_LENGTH ) { return E_PASSWORD; }
	std::memcpy(msg.password.str, p, msg.password.len);


	memcpy(gate_path + gate_dir_len, msg.session_name.str, msg.session_name.len);
	gate_path[gate_dir_len + msg.session_name.len] = '\0';  // NULL終端を付ける
std::cerr << gate_path << std::endl;
	int gate = connect_gate(gate_path);
	if( gate < 0 ) { return E_SESSION_NAME; }

	if( sendfd(gate, guest, &msg, sizeof(msg)) < 0 ) {
		perror("sendfd");
		return E_SENDFD;
	}
	close(guest);
	return E_SUCCESS;
}


}  // namespace Partty

