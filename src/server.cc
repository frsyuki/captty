/*
 * This file is part of Partty
 *
 * Copyright (C) 2007-2008 FURUHASHI Sadayuki
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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include "server.h"
#include "uniext.h"
#include "fdtransport.h"
#include <mp/functional.h>
#include <mp/ios/rw.h>
#include <mp/ios/net.h>

namespace Partty {


Server::Server(config_t& config) :
	impl(new ServerIMPL(config)) {}

ServerIMPL::ServerIMPL(Server::config_t& config) :
	sock(config.listen_socket),
	m_archive_dir(config.archive_dir),
	m_end(0)
{
	gate_dir_len = strlen(config.gate_dir);
	if( gate_dir_len > PATH_MAX ) {
		throw initialize_error("gate directory path is too long");
	}
	memcpy(gate_path, config.gate_dir, gate_dir_len);
}


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
	while(!m_end) {
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

void Server::signal_end(void) { impl->signal_end(); }
void ServerIMPL::signal_end(void) { m_end = 1; }

void ScopedLobby::forked_destroy(int fd)
{
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
		perror("connection closed");
		remove_host(fd, buf);
		return 0;
	}

	negotiation_header_t* header = reinterpret_cast<negotiation_header_t*>(buf);
	if( memcmp(NEGOTIATION_MAGIC_STRING, header->magic, NEGOTIATION_MAGIC_STRING_LENGTH) != 0 ) {
		perror("protocol mismatch");
		send_error_reply(fd, buf, negotiation_reply::PROTOCOL_MISMATCH, "protocol mismatch");
		return 0;
	}

	// ここでエンディアンを直す
	header->message_length = ntohs(header->message_length);
	header->session_name_length = ntohs(header->session_name_length);
	header->writable_password_length = ntohs(header->writable_password_length);
	header->readonly_password_length = ntohs(header->readonly_password_length);

	if( header->protocol_version != PROTOCOL_VERSION ) {
		perror("protocol version mismatch");
		send_error_reply(fd, buf, negotiation_reply::PROTOCOL_MISMATCH, "protocol version mismatch");
		return 0;
	}

	if( header->message_length    > MAX_MESSAGE_LENGTH     ||
	    header->session_name_length > MAX_SESSION_NAME_LENGTH  ||
	    header->session_name_length < MIN_SESSION_NAME_LENGTH  ||
	    header->writable_password_length > MAX_PASSWORD_LENGTH ||
	    header->readonly_password_length > MAX_PASSWORD_LENGTH ||
	    header->session_name_length == 0 ) {
		perror("authentication failed");
		send_error_reply(fd, buf, negotiation_reply::AUTHENTICATION_FAILED, "authentication failed");
		return 0;
	}

	using namespace mp::placeholders;
	if( mp::ios::ios_read_just(mp, fd, (char*)buf + sizeof(negotiation_header_t),
			header->message_length +
				header->session_name_length +
				header->writable_password_length +
				header->readonly_password_length,
			mp::bind(&Lobby::io_fork, this, _1, _2, _3, _4, _5)) < 0 ) {
		perror("multiplexer broken");
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
		perror("connection closed");
		remove_host(fd, buf);
		return 0;
	}

	negotiation_header_t* header = reinterpret_cast<negotiation_header_t*>(buf);

	next_host->fd = fd;

	next_host->message_length = header->message_length;
	next_host->session_name_length = header->session_name_length;
	next_host->writable_password_length = header->writable_password_length;
	next_host->readonly_password_length = header->readonly_password_length;

	std::memcpy( next_host->message,
		     payload,
		     header->message_length);
	next_host->message[header->message_length] = '\0';

	std::memcpy( next_host->session_name,
		     (char*)payload + next_host->message_length,
		     header->session_name_length);
	next_host->session_name[header->session_name_length] = '\0';

	std::memcpy( next_host->writable_password,
		     (char*)payload + next_host->message_length
				    + next_host->session_name_length,
		     header->writable_password_length);
	next_host->writable_password[header->writable_password_length] = '\0';

	std::memcpy( next_host->readonly_password,
		     (char*)payload + next_host->message_length
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


namespace {
void write_info(std::ostream& stream, host_info_t& info)
{
	stream << "session-name: ";
	stream.write(info.session_name, info.session_name_length) << "\n";

	stream << "message: ";
	stream.write(info.message, info.message_length) << "\n";

	stream << "view-only-password: ";
	stream.write(info.readonly_password, info.readonly_password_length) << "\n";

	stream << "operation-password: ";
	stream.write(info.readonly_password, info.readonly_password_length) << "\n";
}

class scoped_ostream {
public:
	scoped_ostream() : m_stream(NULL) {}
	~scoped_ostream() { delete m_stream; }
	void set(std::ostream* stream) { m_stream = stream; }
	std::ostream* ptr(void) { return m_stream; }
private:
	std::ostream* m_stream;
};

}  // noname namespace

int ServerIMPL::run_multiplexer(host_info_t& info)
{
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
						"the session can't be used");
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
		scoped_ostream record;
		{
			std::string archive_base_dir;
			create_archive_base_dir(archive_base_dir);

			std::ostringstream record_info_path;
			record_info_path << archive_base_dir;
			record_info_path.write(info.session_name, info.session_name_length);
			record_info_path << "-info";

			std::ofstream record_info(record_info_path.str().c_str());
			write_info(record_info, info);

			std::ostringstream record_path;
			record_path << archive_base_dir;
			record_path.write(info.session_name, info.session_name_length);

			record.set( new std::ofstream(record_path.str().c_str()) );
		}

		session_info_ref_t ref(info);
		Multiplexer::config_t config(info.fd, gate, ref);
		config.capture_stream = record.ptr();

		Multiplexer multiplexer(config);
		if( sync_reply(fd, negotiation_reply::SUCCESS, "ok") < 0 ) {
			throw io_error("sync reply failed");
		}

		std::cerr << "session ";
		std::cerr.write(info.session_name, info.session_name_length);
		std::cerr << " : ";
		std::cerr.write(info.message, info.message_length);
		std::cerr << std::endl;

		ret = multiplexer.run();

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


void ServerIMPL::create_archive_base_dir(std::string& result)
{
	time_t now = time(NULL);
	struct tm* tmp = gmtime(&now);

	std::ostringstream archive_base_dir;
	archive_base_dir << m_archive_dir;

	archive_base_dir << '/'
		<< std::setw(4) << std::setfill('0') << (tmp->tm_year + 1900);
	mkdir(archive_base_dir.str().c_str(), 0755);

	archive_base_dir << '/'
		<< std::setw(2) << std::setfill('0') << (tmp->tm_mon + 1);
	mkdir(archive_base_dir.str().c_str(), 0755);

	archive_base_dir << '/'
		<< std::setw(2) << std::setfill('0') << tmp->tm_mday;
	mkdir(archive_base_dir.str().c_str(), 0755);

	archive_base_dir << '/'
		<< std::setw(2) << tmp->tm_hour << '-'
		<< std::setw(2) << tmp->tm_min  << '-'
		<< std::setw(2) << tmp->tm_sec  << '-';

	result = archive_base_dir.str();
}


}  // namespace Partty

