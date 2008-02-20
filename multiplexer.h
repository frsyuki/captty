#include <mp/event.h>
#include <mp/sparse_array.h>
#include "emtelnet.h"
#include "partty.h"

namespace Partty {

class filt_telnetd : public emtelnet {
public:
	struct buffer_t {
		const char* buf;
		size_t len;
	};
public:
	filt_telnetd();
	inline void send(const void* buf, size_t len, buffer_t* out);
	inline void recv(const void* buf, size_t len, buffer_t* in, buffer_t* out);
	inline void iclear(void);
	inline void oclear(void);
	inline void iconsumed(size_t len);
	inline void oconsumed(size_t len);
	inline bool is_oempty(void) { return olength == 0; }
	inline bool is_iempty(void) { return ilength == 0; }
	inline void get_ibuffer(buffer_t* in);
	inline void get_obuffer(buffer_t* out);
private:
	static void pass_through_handler(char cmd, bool sw, emtelnet& base) {}
};

void filt_telnetd::send(const void* buf, size_t len, buffer_t* out)
{
	emtelnet::send(buf, len);
	if( out ) { get_obuffer(out); }
}

void filt_telnetd::recv(const void* buf, size_t len, buffer_t* in, buffer_t* out)
{
	emtelnet::recv(buf, len);
	if( out ) { get_obuffer(out); }
	if( in ) { get_ibuffer(in); }
}

void filt_telnetd::iconsumed(size_t len) {
	ilength -= len;
	std::memmove(ibuffer, ibuffer+len, ilength);
}
void filt_telnetd::oconsumed(size_t len) {
	// FIXME 効率が悪い
	// でもほとんど呼ばれない
	olength -= len;
	std::memmove(obuffer, obuffer+len, olength);
}

void filt_telnetd::iclear(void) { ilength = 0; }
void filt_telnetd::oclear(void) { olength = 0; }

void filt_telnetd::get_ibuffer(buffer_t* in) {
	in->buf = ibuffer;
	in->len = ilength;
}
void filt_telnetd::get_obuffer(buffer_t* out) {
	out->buf = obuffer;
	out->len = olength;
}



class MultiplexerIMPL {
public:
	MultiplexerIMPL(int host_socket, int gate_socket,
		const char* session_name, size_t session_name_length,
		const char* password, size_t password_length);
	~MultiplexerIMPL();
	int run(void);
private:
	int host;
	int gate;

	typedef mp::event<void> mpevent;
	mpevent mpev;

	typedef mp::sparse_array<filt_telnetd> guest_set_t;
	guest_set_t guest_set;
	int num_guest;

	static const size_t SHARED_BUFFER_SIZE = 32 * 1024;
	char shared_buffer[SHARED_BUFFER_SIZE];

	char m_session_name[MAX_SESSION_NAME_LENGTH];
	char m_password[MAX_PASSWORD_LENGTH];
	size_t m_session_name_length;
	size_t m_password_length;
private:
	int io_gate(int fd, short event);
	int io_host(int fd, short event);
	int io_guest(int fd, short event);
	int recv_filter(int fd, const void* buf, size_t len, filt_telnetd::buffer_t* ibuf);
	int send_to_guest(int fd, const void* buf, size_t len);
	int guest_try_write(int fd, filt_telnetd& srv);
	void remove_guest(int fd);
	void remove_guest(int fd, filt_telnetd& srv);
private:
	MultiplexerIMPL();
	MultiplexerIMPL(const MultiplexerIMPL&);
};


}  // namespace Partty

