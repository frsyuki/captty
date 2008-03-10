/*
 * Partty
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

#ifndef PARTTY_H__
#define PARTTY_H__ 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

#ifndef PARTTY_GATE_DIR
#define PARTTY_GATE_DIR "./var/socket/"
#endif

#ifndef PARTTY_ARCHIVE_DIR
#define PARTTY_ARCHIVE_DIR \
	"./var/archive"
#endif

#ifndef PARTTY_GATE_SESSION_BANNER
#define PARTTY_GATE_SESSION_BANNER \
	"Welcome to Partty hogehoge!\r\n\r\nSession name: "
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

#ifndef PARTTY_HOST_ENV_NAME
#define PARTTY_HOST_ENV_NAME \
	"PARTTY_SESSION"
#endif


#define PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_SUPPORT
#ifndef PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_POLICY
#define PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_POLICY \
	"<cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\"/></cross-domain-policy>"
#endif


namespace Partty {


static const unsigned short SERVER_DEFAULT_PORT   = 2750;
static const unsigned short GATE_DEFAULT_PORT     = 7777;

static const size_t MAX_SESSION_NAME_LENGTH = 256;
static const size_t MIN_SESSION_NAME_LENGTH = 3;
static const size_t MAX_PASSWORD_LENGTH     = 128;
static const size_t MAX_MESSAGE_LENGTH    = 128;


static const char* const SERVER_WELCOME_MESSAGE = PARTTY_SERVER_WELCOME_MESSAGE;
static const char* const SESSION_START_MESSAGE = PARTTY_SESSION_START_MESSAGE;
static const char* const SESSION_END_MESSAGE = PARTTY_SESSION_END_MESSAGE;


static const uint8_t PROTOCOL_VERSION = 2;

static const char   NEGOTIATION_MAGIC_STRING[] = "Partty!";
static const size_t NEGOTIATION_MAGIC_STRING_LENGTH = 7;

struct negotiation_header_t {
	char magic[NEGOTIATION_MAGIC_STRING_LENGTH];  // "Partty!"
	uint8_t protocol_version;
	uint16_t message_length;
	uint16_t session_name_length;
	uint16_t writable_password_length;
	uint16_t readonly_password_length;
	// char message[message_length];
	// char session_name[session_name_length];
	// char writable_password[writable_password_length];
	// char readonly_password[readonly_password_length];
};

namespace negotiation_reply {
	static const uint16_t SUCCESS = 0;
	static const uint16_t PROTOCOL_MISMATCH = 1;
	static const uint16_t SESSION_UNAVAILABLE = 2;
	static const uint16_t SERVER_ERROR = 3;
	static const uint16_t AUTHENTICATION_FAILED = 4;
};
struct negotiation_reply_t {
	uint16_t code;
	uint16_t message_length;
	// char message[message_length];
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


struct session_info_t;
struct session_info_ref_t {
	explicit inline session_info_ref_t(const session_info_t& src);
	session_info_ref_t() {}
	uint16_t message_length;
	uint16_t session_name_length;
	uint16_t writable_password_length;
	uint16_t readonly_password_length;
	const char* message;
	const char* session_name;
	const char* writable_password;
	const char* readonly_password;
};

struct session_info_t {
	explicit inline session_info_t(const session_info_ref_t& ref);
	session_info_t() {}
	uint16_t message_length;
	uint16_t session_name_length;
	uint16_t writable_password_length;
	uint16_t readonly_password_length;
	char message[MAX_MESSAGE_LENGTH];
	char session_name[MAX_SESSION_NAME_LENGTH];
	char writable_password[MAX_PASSWORD_LENGTH];
	char readonly_password[MAX_PASSWORD_LENGTH];
};

session_info_ref_t::session_info_ref_t(const session_info_t& src) :
	message_length           (src.message_length          ),
	session_name_length      (src.session_name_length     ),
	readonly_password_length (src.readonly_password_length),
	writable_password_length (src.writable_password_length),
	message                  (src.message                 ),
	session_name             (src.session_name            ),
	readonly_password        (src.readonly_password       ),
	writable_password        (src.writable_password       ) {}

session_info_t::session_info_t(const session_info_ref_t& ref) :
	message_length           (ref.message_length          ),
	session_name_length      (ref.session_name_length     ),
	writable_password_length (ref.writable_password_length),
	readonly_password_length (ref.readonly_password_length)
{
	if( message_length > MAX_MESSAGE_LENGTH ) {
		throw initialize_error("message is too long");
	}
	if( session_name_length > MAX_SESSION_NAME_LENGTH ) {
		throw initialize_error("session name is too long");
	}
	if( session_name_length < MIN_SESSION_NAME_LENGTH ) {
		throw initialize_error("session name is too short");
	}
	if( writable_password_length > MAX_PASSWORD_LENGTH ) {
		throw initialize_error("operation password is too long");
	}
	if( readonly_password_length > MAX_PASSWORD_LENGTH ) {
		throw initialize_error("view-only password is too long");
	}
	std::memcpy(message,           ref.message,           ref.message_length);
	std::memcpy(session_name,      ref.session_name,      ref.session_name_length);
	std::memcpy(writable_password, ref.writable_password, ref.writable_password_length);
	std::memcpy(readonly_password, ref.readonly_password, ref.readonly_password_length);
}


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
	struct config_t {
		config_t(int listen_socket_) :
			gate_dir(PARTTY_GATE_DIR),
			archive_dir(PARTTY_ARCHIVE_DIR),
			listen_socket(listen_socket_) {}
	public:
		const char* gate_dir;
		const char* archive_dir;
	private:
		int listen_socket;
		friend class ServerIMPL;
		config_t();
	};
public:
	Server(config_t& config);
	~Server();
	int run(void);
	void signal_end(void);
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
	struct config_t {
		config_t(int host_socket_, int gate_socket_,
				const session_info_ref_t& info_) :
			capture_stream(NULL),
			host_socket(host_socket_),
			gate_socket(gate_socket_),
			info(info_) {}
	public:
		std::ostream* capture_stream;
	private:
		int host_socket;
		int gate_socket;
		const session_info_ref_t& info;
		friend class MultiplexerIMPL;
		config_t();
	};
public:
	Multiplexer(config_t& config);
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
	struct config_t {
		config_t(int _server_socket,
				const session_info_ref_t& info_) :
			lock_code(0),
			server_socket(_server_socket),
			info(info_) {}
	public:
		int lock_code;
	private:
		int server_socket;
		const session_info_ref_t& info;
		friend class HostIMPL;
		config_t();
	};
public:
	Host(config_t& config);
	~Host();
	int run(char* cmd[] = NULL);
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
	struct config_t {
		config_t(int listen_socket_) :
			gate_dir(PARTTY_GATE_DIR),
			session_banner(PARTTY_GATE_SESSION_BANNER),
			password_banner(PARTTY_GATE_PASSWORD_BANNER),
			listen_socket(listen_socket_) {}
	public:
		const char* gate_dir;
		const char* session_banner;
		const char* password_banner;
	private:
		int listen_socket;
		friend class GateIMPL;
		config_t();
	};
public:
	Gate(config_t& config);
	~Gate();
	int run(void);
	void signal_end(void);
private:
	GateIMPL* impl;
	Gate();
	Gate(const Gate&);
};


// プロトコルにTelnetではなくRawストリームを
// 使うGate
class RawGateIMPL;
class RawGate {
public:
	struct config_t {
		config_t(int listen_socket_) :
			gate_dir(PARTTY_GATE_DIR),
#ifdef PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_SUPPORT
			flash_cross_domain_policy(PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_POLICY),
#endif
			listen_socket(listen_socket_) {}
	public:
		const char* gate_dir;
#ifdef PARTTY_RAW_GATE_FLASH_CROSS_DOMAIN_SUPPORT
		const char* flash_cross_domain_policy;
#endif
	private:
		int listen_socket;
		friend class RawGateIMPL;
		config_t();
	};
	RawGate(config_t& config);
	~RawGate();
	int run(void);
	void signal_end(void);
private:
	RawGateIMPL* impl;
	RawGate();
	RawGate(const RawGate&);
};


}  // namespace Partty

#endif /* partty.h */

