#include "partty.h"
#include "emtelnet.h"

namespace Partty {


class phrase_telnetd : public emtelnet {
private:
	static void pass_through_handler(char cmd, bool sw, emtelnet& base) {}
public:
	phrase_telnetd() : emtelnet((void*)this) {
		// use these options
		// force clinet line mode
		//set_my_option_handler( emtelnet::OPT_SGA,
		//		phrase_telnetd::pass_through_handler );
		set_my_option_handler( emtelnet::OPT_BINARY,
				phrase_telnetd::pass_through_handler );

		// supported partner options
		set_partner_option_handler( emtelnet::OPT_ECHO,
				phrase_telnetd::pass_through_handler );
		set_partner_option_handler( emtelnet::OPT_LINEMODE,
				phrase_telnetd::pass_through_handler );
		set_partner_option_handler( emtelnet::OPT_BINARY,
				phrase_telnetd::pass_through_handler );

		//send_will(emtelnet::OPT_SGA);
	}
private:
	phrase_telnetd(const phrase_telnetd&);
};


class GateIMPL {
public:
	GateIMPL(int listen_socket);
	~GateIMPL();
	int run(void);
private:
	int accept_guest(void);
	static ssize_t write_message(phrase_telnetd& td, int guest, const char* buf, size_t count);
	static ssize_t read_line(phrase_telnetd& td, int guest, char* buf, size_t count);
private:
	int socket;
	char gate_path[PATH_MAX + MAX_SESSION_NAME_LENGTH];
	size_t gate_dir_len;
private:
	GateIMPL();
	GateIMPL(const GateIMPL&);
private:
	static const int E_SUCCESS = 0;
	static const int E_ACCEPT = 1;
	static const int E_FINISH = 2;
	static const int E_SENDFD = 3;
	static const int E_SESSION_NAME   = 4;
	static const int E_PASSWORD = 5;
};


}  // namespace Partty

