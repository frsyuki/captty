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

#ifndef PARTTY_PTY_MAKE_RAW_H__
#define PARTTY_PTY_MAKE_RAW_H__

#include <termios.h>
#include <unistd.h>

namespace Partty {


class scoped_pty_make_raw {
public:
	scoped_pty_make_raw(int fd) : m_fd(fd) {
		tcgetattr(m_fd, &m_orig);
		struct termios raw = m_orig;

		//cfmakeraw(&raw);
		raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
				| INLCR | IGNCR | ICRNL | IXON);
		raw.c_oflag &= ~OPOST;
		raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
		raw.c_cflag &= ~(CSIZE | PARENB);
		raw.c_cflag |= CS8;

		//raw.c_lflag &= ~ECHO;
		tcsetattr(m_fd, TCSAFLUSH, &raw);
	}
	~scoped_pty_make_raw() {
		finish();
	}
	void finish(void) {
		if(m_fd >= 0) {
			// XXX: TCSANOW?
			tcsetattr(m_fd, TCSADRAIN, &m_orig);
			write(m_fd, "\n", 1);
			m_fd = -1;
		}
	}
private:
	int m_fd;
	struct termios m_orig;
private:
	scoped_pty_make_raw();
	scoped_pty_make_raw(const scoped_pty_make_raw&);
};


}  // namespace Partty

#endif /* pty_make_raw.h */

