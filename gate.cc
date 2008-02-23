#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/wait.h>
#include "gate.h"
#include "fdtransport.h"
#include "uniext.h"

#include <iostream>

namespace Partty {


phrase_telnetd::phrase_telnetd() : emtelnet((void*)this) {
	// use these options
	// force clinet line mode
	//set_my_option_handler( emtelnet::OPT_SGA,
	//		phrase_telnetd::pass_through_handler );
	set_my_option_handler( emtelnet::OPT_BINARY,
			phrase_telnetd::pass_through_handler );

	// supported partner options
	set_partner_option_handler( emtelnet::OPT_ECHO,
			phrase_telnetd::pass_through_handler );
	set_partner_option_handler( emtelnet::OPT_LINEMODE,
			phrase_telnetd::pass_through_handler );
	set_partner_option_handler( emtelnet::OPT_BINARY,
			phrase_telnetd::pass_through_handler );

	//send_will(emtelnet::OPT_SGA);
}


Gate::Gate(int listen_socket) : impl(new GateIMPL(listen_socket)) {}
GateIMPL::GateIMPL(int listen_socket) :
		socket(listen_socket), m_end(0)
{
	gate_dir_len = strlen(GATE_DIR);
	if( gate_dir_len > PATH_MAX ) {
		throw initialize_error("gate directory path is too long");
	}
	memcpy(gate_path, GATE_DIR, gate_dir_len);
	// この時点ではgate_pathにはNULL終端が付いていない
}

Gate::~Gate() { delete impl; }
GateIMPL::~GateIMPL() {}

int Gate::run(void) { return impl->run(); }
int GateIMPL::run(void)
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

int GateIMPL::accept_guest(void)
{
	int guest = accept(socket, NULL, NULL);
	if( guest  < 0 ) { return E_ACCEPT; }
	if( guest == 0 ) { return E_FINISH; }

	phrase_telnetd td;
	gate_message_t msg;
	ssize_t rl;

	// read user name with linemode
	//td.send_do(emtelnet::OPT_ECHO);
	//td.send_wont(emtelnet::OPT_ECHO);
	//td.send_will(emtelnet::OPT_LINEMODE);
	//td.send_do(emtelnet::OPT_LINEMODE);
	if( write_message(td, guest, GATE_SESSION_BANNER,
				strlen(GATE_SESSION_BANNER)) < 0 ) {
		return E_SESSION_NAME;
	}
	rl = read_line(td, guest, msg.session_name.str, MAX_SESSION_NAME_LENGTH);
	if( rl < 0 ) {
		return E_SESSION_NAME;
	}
	msg.session_name.len = static_cast<size_t>(rl);


	// read password with linemode
	//td.send_wont(emtelnet::OPT_ECHO);
	td.send_will(emtelnet::OPT_ECHO);
	td.send_dont(emtelnet::OPT_ECHO);
	if( write_message(td, guest, GATE_PASSWORD_BANNER,
				strlen(GATE_PASSWORD_BANNER)) < 0 ) {
		return E_PASSWORD;
	}
	rl = read_line(td, guest, msg.password.str, MAX_PASSWORD_LENGTH);
	if( rl < 0 ) {
		return E_PASSWORD;
	}
	msg.password.len = static_cast<size_t>(rl);


	// flush the buffer
	if( write_message(td, guest, "\r\n", 1) < 0 ) {
		return E_PASSWORD;
	}

	memcpy(gate_path + gate_dir_len, msg.session_name.str, msg.session_name.len);
	gate_path[gate_dir_len + msg.session_name.len] = '\0';  // NULL終端を付ける
	std::cout << gate_path << std::endl;
	int gate = connect_gate(gate_path);
	if( gate < 0 ) { return E_SESSION_NAME; }

	if( sendfd(gate, guest, &msg, sizeof(msg)) < 0 ) {
		perror("sendfd");
		return E_SENDFD;
	}
	close(guest);
	return E_SUCCESS;
}


// telnetdを経由してバッファを書き込む
ssize_t GateIMPL::write_message(phrase_telnetd& td, int guest, const char* buf, size_t count)
{
	td.send(buf, count);
	size_t olen = td.olength;
	td.clear_obuffer();
	if( olen > 0 ) {
		if( write_all(guest, td.obuffer, olen) != olen ) {
			return -1;
		}
	}
	return olen;
}


// 1行読み込んで余剰分を捨てる
ssize_t GateIMPL::read_line(phrase_telnetd& td, int guest, char* buf, size_t count)
{
	char* p = buf;
	char* endp = p + count;
	ssize_t len;
	while(p < endp) {
		len = read(guest, p, endp - p);
		if( len < 0 ) {
			if( errno != EAGAIN || errno != EINTR ) {
				return -1;
			}
		} else if( len == 0 ) {
			return -2;
		} else {
			td.recv(p, len);
			char* c = td.ibuffer;
			char* cend = c + td.ilength;
			td.clear_ibuffer();
			for(; c < cend; ++c, ++p) {
				if( *c < 0x21 || *c > 0x7E ) {
					// *c is not printable
					*p = '\0';
					return p - buf;
				}
				*p = *c;
			}
			if( td.olength > 0 ) {
				size_t olen = td.olength;
				td.clear_obuffer();
				if( write_all(guest, td.obuffer, olen) != olen ) {
					return -3;
				}
			}
		}
	}
	return -3;  // too long
}


}  // namespace Partty

