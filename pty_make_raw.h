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

