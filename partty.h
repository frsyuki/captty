#ifndef PARTTY_H__
#define PARTTY_H__ 1

#include <stddef.h>

#ifndef PARTTY_GATE_DIR
#define PARTTY_GATE_DIR "./var/"
#endif

//#ifndef PARTTY_GATE_SOCKET_NAME
//#define PARTTY_GATE_SOCKET_NAME "gate"
//#endif

#ifndef PARTTY_GATE_SESSION_BANNER
#define PARTTY_GATE_SESSION_BANNER \
	"Welcome to Partty hogehoge!\n\nSession name: "
#endif

#ifndef PARTTY_GATE_PASSWORD_BANNER
#define PARTTY_GATE_PASSWORD_BANNER \
	"Password: "
#endif

#ifndef PARTTY_SERVER_WELCOME_MESSAGE
#define PARTTY_SERVER_WELCOME_MESSAGE \
	"\r\n>Partty connected!\r\n"
#endif

namespace Partty {


static const size_t MAX_SESSION_NAME_LENGTH = 64;
static const size_t MAX_PASSWORD_LENGTH = 64;

static const char* const GATE_DIR = PARTTY_GATE_DIR;
//static const char* const GATE_SOCKET_NAME = PARTTY_GATE_SOCKET_NAME;
static const char* const GATE_PASSWORD_BANNER = PARTTY_GATE_PASSWORD_BANNER;
static const char* const GATE_SESSION_BANNER = PARTTY_GATE_SESSION_BANNER;
static const char* const SERVER_WELCOME_MESSAGE = PARTTY_SERVER_WELCOME_MESSAGE;


template <size_t Length>
struct line_t {
	char str[Length];
	size_t len;
};
struct gate_message_t {
	line_t<MAX_SESSION_NAME_LENGTH> session_name;
	line_t<MAX_PASSWORD_LENGTH> password;
};


class ServerIMPL;
class Server {
public:
	Server(	int host_socket, int gate_socket,
		const char* session_name, size_t session_name_length,
		const char* password, size_t password_length );
	~Server();
public:
	int run(void);
private:
	ServerIMPL* impl;
private:
	Server();
	Server(const Server&);
};


class HostIMPL;
class Host {
public:
	Host(int server_socket);
	~Host();
public:
	int run(void);
private:
	HostIMPL* impl;
private:
	Host();
	Host(const Host&);
};



class GateIMPL;
class Gate {
public:
	Gate(int listen_socket);
	~Gate();
public:
	int run(void);
private:
	GateIMPL* impl;
private:
	Gate();
	Gate(const Gate&);
};



}  // namespace Partty

#endif /* partty.h */

