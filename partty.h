#ifndef PARTTY_H__
#define PARTTY_H__ 1

#include <stddef.h>
#include <stdint.h>
#include <stdexcept>

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
	"\r\n>Partty! connected.\r\n"
#endif

#ifndef PARTTY_SESSION_START_MESSAGE
#define PARTTY_SESSION_START_MESSAGE \
	">Partty! start\r\n"
#endif

#ifndef PARTTY_SESSION_END_MESSAGE
#define PARTTY_SESSION_END_MESSAGE \
	">Partty! end"
#endif

namespace Partty {


static const unsigned short SERVER_DEFAULT_PORT   = 2750;
static const unsigned short GATE_DEFAULT_PORT     = 7777;

static const size_t MAX_SESSION_NAME_LENGTH = 128;
static const size_t MIN_SESSION_NAME_LENGTH = 3;
static const size_t MAX_PASSWORD_LENGTH     = 128;
static const size_t MAX_USER_NAME_LENGTH    = 128;


static const char* const GATE_DIR = PARTTY_GATE_DIR;
//static const char* const GATE_SOCKET_NAME = PARTTY_GATE_SOCKET_NAME;
static const char* const GATE_PASSWORD_BANNER = PARTTY_GATE_PASSWORD_BANNER;
static const char* const GATE_SESSION_BANNER = PARTTY_GATE_SESSION_BANNER;
static const char* const SERVER_WELCOME_MESSAGE = PARTTY_SERVER_WELCOME_MESSAGE;
static const char* const SESSION_START_MESSAGE = PARTTY_SESSION_START_MESSAGE;
static const char* const SESSION_END_MESSAGE = PARTTY_SESSION_END_MESSAGE;


static const char VERSION[] = "0.0";
static const char REVISION[] = "0";
static const uint8_t PROTOCOL_VERSION = 0;

static const char   NEGOTIATION_MAGIC_STRING[] = "Partty!";
static const size_t NEGOTIATION_MAGIC_STRING_LENGTH = 7;

struct negotiation_header_t {
	char magic[NEGOTIATION_MAGIC_STRING_LENGTH];  // "Partty!"
	uint8_t protocol_version;
	uint16_t user_name_length;
	uint16_t session_name_length;
	uint16_t password_length;
	// char user_name[user_name_length];
	// char session_name[session_name_length];
	// char password[password_length];
};

namespace negotiation_reply {
	static const uint16_t SUCCESS = 0;
	static const uint16_t PROTCOL_MISMATCH = 1;
	static const uint16_t ADDRESS_ALREADY_IN_USE = 2;
	static const uint16_t SERVER_ERROR = 3;
	static const uint16_t AUTHENTICATION_FAILED = 4;
};
struct negotiation_reply_t {
	uint16_t code;
	uint16_t message_length;
	// char message[message_length];
};


template <size_t Length>
struct line_t {
	char str[Length];
	size_t len;
};
struct gate_message_t {
	line_t<MAX_SESSION_NAME_LENGTH> session_name;
	line_t<MAX_PASSWORD_LENGTH> password;
};


// Hostを待ち受け、Multiplexerをforkする
class ServerIMPL;
class Server {
public:
	Server(int listen_socket);
	~Server();
	int run(void);
private:
	ServerIMPL* impl;
	Server();
	Server(const Server&);
};


// Hostからの入力をゲストにコピーする
// ゲストからの入力をHostにコピーする
class MultiplexerIMPL;
class Multiplexer {
public:
	Multiplexer(int host_socket, int gate_socket,
		const char* session_name, size_t session_name_length,
		const char* password, size_t password_length);
	~Multiplexer();
	int run(void);
private:
	MultiplexerIMPL* impl;
	Multiplexer();
	Multiplexer(const Multiplexer&);
};


// 新しい仮想端末を確保して、シェルを起動する
// 標準入力とServerからの入力をシェルにコピーする
// シェルの出力と標準出力をServerにコピーする
class HostIMPL;
class Host {
public:
	Host(int server_socket, char lock_code,
		const char* session_name, size_t session_name_length,
		const char* password, size_t password_length);
	~Host();
	int run(void);
private:
	HostIMPL* impl;
	Host();
	Host(const Host&);
};


// ゲストを待ち受け、ファイルディスクリプタを
// Hostに転送する
class GateIMPL;
class Gate {
public:
	Gate(int listen_socket);
	~Gate();
	int run(void);
private:
	GateIMPL* impl;
	Gate();
	Gate(const Gate&);
};


struct partty_error : public std::runtime_error {
	partty_error(const std::string& message) : runtime_error(message) {}
	virtual ~partty_error() throw() {}
};

struct initialize_error : public partty_error {
	initialize_error(const std::string& message) : partty_error(message) {}
	virtual ~initialize_error() throw() {}
};

struct io_error : public partty_error {
	io_error(const std::string& message) : partty_error(message) {}
	virtual ~io_error() throw() {}
};

struct io_end_error : public io_error {
	io_end_error(const std::string& message) : io_error(message) {}
	virtual ~io_end_error() throw() {}
};


}  // namespace Partty

#endif /* partty.h */

