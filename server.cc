#include "server.h"
#include <mp/functional.h>
#include <mp/ios/rw.h>
#include <mp/ios/net.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "uniext.h"
#include "fdtransport.h"

namespace Partty {


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
	if( fcntl(sock, F_SETFL, O_NONBLOCK) < 0 ) {
		throw initialize_error("failed to set listen socket nonblock mode");
	}
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
	// FIXME シグナルハンドラ
		int ret = lobby.next(info);
		if( ret < 0 ) {
			perror("multiplexer is broken");
			throw io_error("multiplexer is borken");
		}
		pid = fork();
		if( pid < 0 ) {
			// error
			perror("failed to fork");
			throw partty_error("failed to fork");
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
	// 標準エラー出力から先を閉じる
	// FIXME 0,1,2は標準入出力でないかもしれない
	for(int i = 3; i <= max_fd; ++i) {
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
	if( buf == NULL ) {
		remove_host(fd);
		return -1;
	}
	if( fd > max_fd ) { max_fd = fd; }
std::cerr << "accepted " << fd << std::endl;
	using namespace mp::placeholders;
	if( mp::ios::ios_read_just(mp, fd, buf, sizeof(negotiation_header_t),
			mp::bind(&Lobby::io_payload, this, _1, _2, _3, _4, _5)) < 0 ) {
		remove_host(fd, buf);
		return -1;
	}
perror("waiting header");
	return 0;
}


int Lobby::io_payload(mpio& mp, int fd, void* buf, size_t just, size_t len)
{
	if( len != just ) {
		remove_host(fd, buf);
		return 0;
	}

	negotiation_header_t* header = reinterpret_cast<negotiation_header_t*>(buf);
	if( memcmp(NEGOTIATION_MAGIC_STRING, header->magic, NEGOTIATION_MAGIC_STRING_LENGTH) != 0 ) {
		send_error_reply(fd, buf, negotiation_reply::PROTOCOL_MISMATCH, "protocol mismatch");
		return 0;
	}

	// ここでエンディアンを直す
	header->user_name_length = ntohs(header->user_name_length);
	header->session_name_length = ntohs(header->session_name_length);
	header->writable_password_length = ntohs(header->writable_password_length);
	header->readonly_password_length = ntohs(header->readonly_password_length);

	if( header->protocol_version != PROTOCOL_VERSION ) {
		send_error_reply(fd, buf, negotiation_reply::PROTOCOL_MISMATCH, "protocol version mismatch");
		return 0;
	}

	if( header->user_name_length    > MAX_USER_NAME_LENGTH     ||
	    header->session_name_length > MAX_SESSION_NAME_LENGTH  ||
	    header->session_name_length < MIN_SESSION_NAME_LENGTH  ||
	    header->writable_password_length > MAX_PASSWORD_LENGTH ||
	    header->readonly_password_length > MAX_PASSWORD_LENGTH ||
	    header->session_name_length == 0 ) {
		send_error_reply(fd, buf, negotiation_reply::AUTHENTICATION_FAILED, "authentication failed");
		return 0;
	}

	using namespace mp::placeholders;
	if( mp::ios::ios_read_just(mp, fd, (char*)buf + sizeof(negotiation_header_t),
			header->user_name_length +
				header->user_name_length +
				header->session_name_length +
				header->writable_password_length +
				header->readonly_password_length,
			mp::bind(&Lobby::io_fork, this, _1, _2, _3, _4, _5)) < 0 ) {
		send_error_reply(fd, buf, negotiation_reply::SERVER_ERROR, "multiplexer is broken");
		return 0;
	}

	return 0;
}


int Lobby::io_error_reply(mpio& mp, int fd, void* buf, size_t just, size_t len)
{
perror("io_error_reply");
std::cerr << len << " " << just << std::endl;
	remove_host(fd, buf);
	return 0;
}


void Lobby::send_error_reply(int fd, void* buf, uint16_t code, const char* message)
{
	send_error_reply(fd, buf, code, message, strlen(message));
}

void Lobby::send_error_reply(int fd, void* buf, uint16_t code, const char* message, size_t message_length)
{
	if( message_length + sizeof(negotiation_reply_t) > BUFFER_SIZE) {
		remove_host(fd, buf);  // FIXME
	}
	negotiation_reply_t* reply = reinterpret_cast<negotiation_reply_t*>(buf);
	reply->code = htons(code);
	reply->message_length = htons(message_length);
	memcpy((char*)buf + sizeof(negotiation_reply_t), message, message_length);
	using namespace mp::placeholders;
	if( mp::ios::ios_write_just(mp, fd, buf, sizeof(negotiation_reply_t) + message_length,
			mp::bind(&Lobby::io_error_reply, this, _1, _2, _3, _4, _5)) < 0 ) {
		remove_host(fd, buf);
	}
}


int Lobby::io_fork(mpio& mp, int fd, void* payload, size_t just, size_t len)
{
	void* buf = (void*)(((char*)payload) - sizeof(negotiation_header_t));
	if( len != just ) {
		remove_host(fd, buf);
		return 0;
	}

	negotiation_header_t* header = reinterpret_cast<negotiation_header_t*>(buf);

	next_host->fd = fd;

	next_host->user_name_length = header->user_name_length;
	next_host->session_name_length = header->session_name_length;
	next_host->writable_password_length = header->writable_password_length;
	next_host->readonly_password_length = header->readonly_password_length;

	std::memcpy( next_host->user_name,
		     payload,
		     header->user_name_length);
	next_host->user_name[header->user_name_length] = '\0';

	std::memcpy( next_host->session_name,
		     (char*)payload + next_host->user_name_length,
		     header->session_name_length);
	next_host->session_name[header->session_name_length] = '\0';

	std::memcpy( next_host->writable_password,
		     (char*)payload + next_host->user_name_length
				    + next_host->session_name_length,
		     header->writable_password_length);
	next_host->writable_password[header->writable_password_length] = '\0';

	std::memcpy( next_host->readonly_password,
		     (char*)payload + next_host->user_name_length
				    + next_host->session_name_length
				    + next_host->readonly_password_length,
		     header->readonly_password_length);
	next_host->readonly_password[header->readonly_password_length] = '\0';

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


int ServerIMPL::sync_reply(int fd, uint16_t code, const char* message)
{
	return sync_reply(fd, code, message, strlen(message));
}

int ServerIMPL::sync_reply(int fd, uint16_t code, const char* message, size_t message_length)
{
	size_t buflen = sizeof(negotiation_reply_t) + message_length;
	char* buf = (char*)malloc(buflen);
	if( buf == NULL ) { return -1; }
	negotiation_reply_t* reply = reinterpret_cast<negotiation_reply_t*>(buf);
	reply->code = htons(code);
	reply->message_length = htons(message_length);
	memcpy((char*)buf + sizeof(negotiation_reply_t), message, message_length);
	if( write_all(fd, buf, buflen) != buflen ) {
		return -1;
	}
	return 0;
}

int ServerIMPL::run_multiplexer(host_info_t& info)
{
	char gate_path[PATH_MAX + Partty::MAX_SESSION_NAME_LENGTH];
	ssize_t gate_dir_len = strlen(Partty::GATE_DIR);
	memcpy(gate_path, Partty::GATE_DIR, gate_dir_len);
	memcpy(gate_path + gate_dir_len, info.session_name, info.session_name_length);
	gate_path[gate_dir_len + info.session_name_length] = '\0';

	int fd = info.fd;

	int gate = listen_gate(gate_path);
	if( gate < 0 ) {
		int testfd = connect_gate(gate_path);
		if( testfd < 0 ) {
			unlink(gate_path);
			gate = listen_gate(gate_path);
			if( gate < 0 ) {
				sync_reply(fd, negotiation_reply::SESSION_UNAVAILABLE,
						"the session is can't be used");
				close(fd);
				return 1;
			}
		} else {
			close(testfd);
			sync_reply(fd, negotiation_reply::SESSION_UNAVAILABLE,
					"the session is already in use");
			close(fd);
			return 1;
		}
	}

	int ret;
	try {
		Multiplexer multiplexer(info.fd, gate, session_info_ref_t(info));
		if( sync_reply(fd, negotiation_reply::SUCCESS, "ok") < 0 ) {
			throw io_error("sync reply failed");
		}
		std::cerr << "session " << info.session_name << " by " << info.user_name << std::endl;
		ret = multiplexer.run();
		unlink(gate_path);
	} catch (initialize_error& e) {
		perror(e.what());
		sync_reply(fd, negotiation_reply::SERVER_ERROR, e.what());
		ret = 1;
	} catch (std::exception& e) {
		perror(e.what());
		ret = 1;
	} catch (...) {
		perror("unknown error");
		ret = 1;
	}
	unlink(gate_path);
	return ret;
}


}  // namespace Partty

