#include <mp/io.h>
#include "partty.h"

namespace Partty {


struct host_info_t {
	int fd;
	size_t user_name_length;
	size_t session_name_length;
	size_t password_length;
	char user_name[MAX_USER_NAME_LENGTH];
	char session_name[MAX_PASSWORD_LENGTH];
	char password[MAX_PASSWORD_LENGTH];
};

class Lobby {
public:
	Lobby(int listen_socket);
	~Lobby();
	int next(host_info_t& info);
	void forked_destroy(int fd);
private:
	static const size_t BUFFER_SIZE =
			sizeof(negotiation_header_t) +
			MAX_USER_NAME_LENGTH +
			MAX_SESSION_NAME_LENGTH +
			MAX_PASSWORD_LENGTH ;
	struct data_t {
		char* buffer;
		char* p;
		size_t clen;
	};
	typedef mp::io<data_t> mpio;
	mpio mp;
private:
	int sock;
	int max_fd;
	host_info_t* next_host;
private:
	int io_header(mpio& mp, int fd);
	int io_payload(mpio& mp, int fd, void* buf, size_t just, size_t len);
	int io_error_reply(mpio& mp, int fd, void* buf, size_t just, size_t len);
	int io_fork(mpio& mp, int fd, void* buf, size_t just, size_t len);
	void send_error_reply(int fd, void* buf, uint16_t code, const char* message);
	void send_error_reply(int fd, void* buf, uint16_t code, const char* message, size_t message_length);
	void remove_host(int fd);
	void remove_host(int fd, void* buf);
};

class ScopedLobby {
public:
	ScopedLobby(int sock) : impl(new Lobby(sock)) {}
	~ScopedLobby() { delete impl; }
	int next(host_info_t& info) { return impl->next(info); }
	void forked_destroy(int fd);
private:
	Lobby* impl;
private:
	ScopedLobby();
	ScopedLobby(const ScopedLobby&);
};


class ServerIMPL {
public:
	ServerIMPL(int listen_socket);
	~ServerIMPL();
	int run(void);
private:
	int sock;
private:
	int sync_reply(int fd, uint16_t code, const char* message);
	int sync_reply(int fd, uint16_t code, const char* message, size_t message_length);
	int run_multiplexer(host_info_t& info);
};


}  // namespace Partty

