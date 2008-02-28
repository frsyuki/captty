#include "partty.h"
#include "emtelnet.h"
#include <string>

namespace Partty {


class phrase_telnetd : public emtelnet {
private:
	static void pass_through_handler(char cmd, bool sw, emtelnet& base) {}
public:
	phrase_telnetd();
private:
	phrase_telnetd(const phrase_telnetd&);
};


class GateIMPL {
public:
	GateIMPL(Gate::config_t& config);
	~GateIMPL();
	int run(void);
	void signal_end(void);
private:
	int accept_guest(void);
	static ssize_t write_message(phrase_telnetd& td, int guest, const char* buf, size_t count);
	static ssize_t read_line(phrase_telnetd& td, int guest, char* buf, size_t count);
private:
	int socket;
	char gate_path[PATH_MAX + MAX_SESSION_NAME_LENGTH];
	size_t gate_dir_len;
	std::string m_session_banner;
	std::string m_password_banner;
	sig_atomic_t m_end;
private:
	GateIMPL();
	GateIMPL(const GateIMPL&);
private:
	static const int E_SUCCESS = 0;
	static const int E_ACCEPT = 1;
	static const int E_FINISH = 2;
	static const int E_SESSION_NAME = 3;
	static const int E_PASSWORD = 4;
	static const int E_SENDFD = 5;
};


}  // namespace Partty

