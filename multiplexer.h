#include <mp/event.h>
#include <mp/sparse_array.h>
#include <vector>
#include "emtelnet.h"
#include "partty.h"
#include "captty.h"

namespace Partty {

class sender_telnetd : public emtelnet {
public:
	sender_telnetd();
private:
	static void pass_through_handler(char cmd, bool sw, emtelnet& base) {}
	static void window_size_handler(char cmd, const char* msg, size_t len, emtelnet& self);
public:
	void ws_change(short cols, short rows) {
		m_cols = cols;
		m_rows = rows;
		m_ws_changed = true;
	}
	bool ws_changed(void) const { return m_ws_changed; }
	short get_cols(void) const { return m_cols; }
	short get_rows(void) const { return m_rows; }
	void ws_flush(void) { m_ws_changed = false; }
	bool ws_initialized(void) const { return (m_cols != 0 && m_rows != 0); }
private:
	short m_cols;
	short m_rows;
	bool m_ws_changed;
};


class filt_telnetd : public emtelnet {
public:
	struct buffer_t {
		const char* buf;
		size_t len;
	};
public:
	explicit filt_telnetd(sender_telnetd& host_telnet);
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
public:
	void send_ws(const char* sbbuf, size_t sz4);
private:
	static void pass_through_handler(char cmd, bool sw, emtelnet& base) {}
	static void enable_ws_handler(char cmd, bool sw, emtelnet& base);
	sender_telnetd& m_host_telnet;
	bool m_enable_ws;
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
	MultiplexerIMPL(Multiplexer::config_t& config);
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

	typedef std::vector<bool> writable_guest_t;
	writable_guest_t writable_guest;

	static const size_t SHARED_BUFFER_SIZE = 32 * 1024;
	char shared_buffer[SHARED_BUFFER_SIZE];

	session_info_t m_info;

	sender_telnetd m_host_telnet;

	std::ostream* m_capture_stream;
	Captty::Recorder *m_recorder;
private:
	int io_gate(int fd, short event);
	int io_host(int fd, short event);
	int io_guest(int fd, short event, bool writable);
	int io_message(int fd, short event);
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

