#include <mp/dispatch.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "partty.h"
#include "emtelnet.h"

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

