#include <mp/dispatch.h>
#include <sys/ioctl.h>
#include "partty.h"

namespace Partty {


class HostIMPL {
public:
	HostIMPL(int server_socket,
		const char* session_name, size_t session_name_length,
		const char* password, size_t password_length);
	~HostIMPL();
	int run(void);
private:
	int sh;
	int host;
	int server;

	char m_session_name[MAX_SESSION_NAME_LENGTH];
	char m_password[MAX_PASSWORD_LENGTH];
	size_t m_session_name_length;
	size_t m_password_length;
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

