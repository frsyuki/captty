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

#include "partty.h"
#include "emtelnet.h"
#include <sys/ioctl.h>
#include <termios.h>
#include <mp/dispatch.h>

namespace Partty {

class receiver_telnetd : public emtelnet {
public:
	receiver_telnetd();
	void send_ws(unsigned short row, unsigned short col);
private:
	static void pass_through_handler(char cmd, bool sw, emtelnet& base) {}
};

class HostIMPL {
public:
	HostIMPL(Host::config_t& config);
	~HostIMPL();
	int run(char* cmd[] = NULL);
private:
	int sh;
	int host;
	int server;

	char m_lock_code;
	bool m_locking;

	bool m_view_only;

	session_info_t m_info;
private:
	HostIMPL();
	HostIMPL(const HostIMPL&);
private:
	int io_stdin(int fd, short event);
	int io_server(int fd, short event);
	int io_shell(int fd, short event);
private:
	typedef mp::dispatch<void> mpdispatch;
	mpdispatch mpdp;
	static const size_t SHARED_BUFFER_SIZE = 32 * 1024;
	char shared_buffer[SHARED_BUFFER_SIZE];
	receiver_telnetd m_telnet;
private:
	struct winsize winsz;
	static int get_window_size(int fd, struct winsize* ws);
	static int set_window_size(int fd, struct winsize* ws);
};


}  // namespace Partty

