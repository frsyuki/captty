#ifndef PARTTY_SCOPED_MAKE_RAW_H__
#define PARTTY_SCOPED_MAKE_RAW_H__ 1

#include <termios.h>

class PtyScopedMakeRaw {
public:
	PtyScopedMakeRaw(int fd) : m_fd(fd) {
		tcgetattr(m_fd, &m_orig);

		struct termios raw = m_orig;
		cfmakeraw(&raw);
		//raw.c_lflag &= ~ECHO;
		tcsetattr(m_fd, TCSAFLUSH, &raw);
		// XXX: エラー処理
	}
	~PtyScopedMakeRaw() {
		// XXX: TCSANOW?
		tcsetattr(m_fd, TCSADRAIN, &m_orig);
		write(m_fd, "\n", 1);
	}
private:
	int m_fd;
	struct termios m_orig;
private:
	PtyScopedMakeRaw();
};

#endif /* scoped_make_raw.h */

