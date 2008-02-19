#include "server.h"
#include <mp/functional.h>
#include <mp/ios/rw.h>
#include <mp/ios/net.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "unio.h"
#include "fdtransport.h"

namespace Partty {
void handler(int) { perror("hoge"); exit(1); }


Server::Server(int listen_socket) :
	impl(new ServerIMPL(listen_socket)) {}
ServerIMPL::ServerIMPL(int listen_socket) :
	sock(listen_socket) {}

Server::~Server() { delete impl; }
ServerIMPL::~ServerIMPL()
{
	close(sock);
}

Lobby::Lobby(int listen_socket) : sock(listen_socket), max_fd(-1)
{
	if( fcntl(sock, F_SETFL, O_NONBLOCK) < 0 )
		{ throw initialize_error("failed to set listen socket nonblock mode"); }
	using namespace mp::placeholders;
	if( mp::ios::ios_accept(mp, sock,
			mp::bind(&Lobby::io_header, this, _1, _2) ) < 0 ) {
		throw initialize_error("can't add listen socket to IO multiplexer");
	}
}

Lobby::~Lobby()
{
	close(sock);
}


int Server::run(void) { return impl->run(); }
int ServerIMPL::run(void)
{
	host_info_t info;

	pid_t pid;
	ScopedLobby lobby(sock);
	while(1) {
		int ret = lobby.next(info);
		if( ret < 0 ) {
			// FIXME
			pexit("lobby.next");
		}
		pid = fork();
		if( pid < 0 ) {
			// error
			// FIXME
			pexit("lobby.next");
		} else if( pid == 0 ) {
			// child
			lobby.forked_destroy(info.fd);
			exit( run_multiplexer(info) );
		}
		// parent
		// accept next host
		close(info.fd);
	}
}

void ScopedLobby::forked_destroy(int fd) {
	impl->forked_destroy(fd);
	delete impl;
	impl = NULL;
}

void Lobby::forked_destroy(int fd)
{
	// FIXME
	for(int i = 0; i <= max_fd; ++i) {
		if( i != fd ) {
			close(i);
		}
	}
}



int Lobby::next(host_info_t& info)
{
	next_host = &info;
	return mp.run();
}


int Lobby::io_header(mpio& mp, int fd)
{
	if( fd < 0 ) { return -1; }
	void* buf = mp.pool.malloc(BUFFER_SIZE);
	if( buf == NULL ) { remove_host(fd); return -1; }
	if( fd > max_fd ) { max_fd = fd; }
std::cerr << "accepted " << fd << std::endl;
	using namespace mp::placeholders;
	if( mp::ios::ios_read_just(mp, fd, buf, sizeof(negotiation_header_t),
			mp::bind(&Lobby::io_payload, this, _1, _2, _3, _4, _5)) < 0 ) {
		remove_host(fd, buf);
		return 0;
	}
perror("waiting header");
	return 0;
}



int Lobby::io_payload(mpio& mp, int fd, void* buf, size_t just, size_t len)
{
perror("header reached");
	if( len != just ) {
		remove_host(fd, buf);
		return 0;
	}

	negotiation_header_t* header = reinterpret_cast<negotiation_header_t*>(buf);
	if( memcmp(NEGOTIATION_MAGIC_STRING, header->magic, NEGOTIATION_MAGIC_STRING_LENGTH) != 0 ) {
		remove_host(fd, buf);
		return 0;
	}

	// ここでエンディアンを直す
	header->user_name_length = ntohs(header->user_name_length);
	header->session_name_length = ntohs(header->session_name_length);
	header->password_length = ntohs(header->password_length);

	if( header->user_name_length    > MAX_USER_NAME_LENGTH    ||
	    header->session_name_length > MAX_SESSION_NAME_LENGTH ||
	    header->password_length     > MAX_PASSWORD_LENGTH     ||
	    header->session_name_length == 0 ) {
		// FIXME Hostにメッセージを返す？
		remove_host(fd, buf);
		return 0;
	}

perror("header ok");
	using namespace mp::placeholders;
	if( mp::ios::ios_read_just(mp, fd, (char*)buf + sizeof(negotiation_header_t),
			header->user_name_length +
				header->session_name_length +
				header->password_length,
			mp::bind(&Lobby::io_fork, this, _1, _2, _3, _4, _5)) < 0 ) {
		remove_host(fd, buf);
		return 0;
	}

std::cerr << "waiting length " << (header->user_name_length + header->session_name_length + header->password_length)
				<< std::endl;
	return 0;
}

int Lobby::io_fork(mpio& mp, int fd, void* payload, size_t just, size_t len)
{
perror("payload reached");
	void* buf = (void*)(((char*)payload) - sizeof(negotiation_header_t));
	if( len != just ) {
		remove_host(fd, buf);
		return 0;
	}

	negotiation_header_t* header = reinterpret_cast<negotiation_header_t*>(buf);

	next_host->fd = fd;

	next_host->user_name_length    = header->user_name_length;
	next_host->session_name_length = header->session_name_length;
	next_host->password_length     = header->password_length;

	std::memcpy( next_host->user_name,
		     payload,
		     header->user_name_length);

	std::memcpy( next_host->session_name,
		     (char*)payload + next_host->user_name_length,
		     header->session_name_length);

	std::memcpy( next_host->password,
		     (char*)payload + next_host->user_name_length
				    + next_host->session_name_length,
		     header->password_length);

perror("payload ok");
	mp.remove(fd);
	mp.pool.free(buf);

	return 2;   // return > 0
}


void Lobby::remove_host(int fd)
{
	mp.remove(fd);
	close(fd);
	if( fd == max_fd ) { --max_fd; }
}

void Lobby::remove_host(int fd, void* buf)
{
	mp.remove(fd);
	close(fd);
	mp.pool.free(buf);
	if( fd == max_fd ) { --max_fd; }
}


int ServerIMPL::run_multiplexer(host_info_t& info)
{
	char gate_path[PATH_MAX + Partty::MAX_SESSION_NAME_LENGTH];
	ssize_t gate_dir_len = strlen(Partty::GATE_DIR);
	memcpy(gate_path, Partty::GATE_DIR, gate_dir_len);
	memcpy(gate_path + gate_dir_len, info.session_name, info.session_name_length);
	gate_path[gate_dir_len + info.session_name_length] = '\0';

	int gate = listen_gate(gate_path);
	if( gate < 0 ) { pexit("listen_gate"); }  // FIXME hostになにか返す
		// the session is already in use

	Multiplexer multiplexer( info.fd, gate,
				 info.session_name, info.session_name_length,
				 info.password, info.password_length);
	int ret = multiplexer.run();
	unlink(gate_path);
	return ret;
}


}  // namespace Partty

