#include <mp/dispatch.h>
#include <sys/ioctl.h>
#include "partty.h"

namespace Partty {


class HostIMPL {
public:
	HostIMPL(Host::config_t& config);
	~HostIMPL();
	int run(void);
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
private:
	struct winsize winsz;
	static int get_window_size(int fd, struct winsize* ws);
	static int set_window_size(int fd, struct winsize* ws);
};


}  // namespace Partty

