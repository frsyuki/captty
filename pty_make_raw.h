#ifndef PARTTY_PTY_MAKE_RAW_H__
#define PARTTY_PTY_MAKE_RAW_H__

#include <termios.h>

namespace Partty {


class scoped_pty_make_raw {
public:
	scoped_pty_make_raw(int fd) : m_fd(fd) {
		tcgetattr(m_fd, &m_orig);
		struct termios raw = m_orig;
		cfmakeraw(&raw);
		//raw.c_lflag &= ~ECHO;
		tcsetattr(m_fd, TCSAFLUSH, &raw);
	}
	~scoped_pty_make_raw() {
		if(m_fd >= 0) {
			// XXX: TCSANOW?
			tcsetattr(m_fd, TCSADRAIN, &m_orig);
			write(m_fd, "\n", 1);
		}
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
