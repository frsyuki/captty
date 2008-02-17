#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bitset>
#include <limits>
#include <mp/event.h>
#include "scoped_make_raw.h"
#include "unio.h"
#include "fdtransport.h"
#include "emtelnet.h"
#include "sparse_array.h"
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

filt_telnetd::filt_telnetd() : emtelnet((void*)this)
{
	// use these options
	set_my_option_handler( emtelnet::OPT_SGA,
			filt_telnetd::pass_through_handler );
	set_my_option_handler( emtelnet::OPT_ECHO,
			filt_telnetd::pass_through_handler );
	set_my_option_handler( emtelnet::OPT_BINARY,
			filt_telnetd::pass_through_handler );

	// supported partner options
	set_partner_option_handler( emtelnet::OPT_BINARY,
			filt_telnetd::pass_through_handler );

	// prevent line mode
	send_will(emtelnet::OPT_SGA);
	send_will(emtelnet::OPT_ECHO);
	send_dont(emtelnet::OPT_ECHO);
	send_dont(emtelnet::OPT_LINEMODE);

	// enable multibyte characters
	send_will(emtelnet::OPT_BINARY);
	send_do(emtelnet::OPT_BINARY);
}

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

	typedef sparse_array<filt_telnetd> guest_set_t;
	guest_set_t guest_set;
	int num_guest;

	static const size_t SHARED_BUFFER_SIZE = 32 * 1024;
	char shared_buffer[SHARED_BUFFER_SIZE];

	char m_session_name[MAX_SESSION_NAME_LENGTH];
	char m_password[MAX_PASSWORD_LENGTH];
	size_t m_session_name_length;
	size_t m_password_length;
private:
	int io_host(int fd, short event);
	int io_gate(int fd, short event);
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



Multiplexer::Multiplexer(int host_socket, int gate_socket,
	const char* session_name, size_t session_name_length,
	const char* password, size_t password_length) :
		impl(new MultiplexerIMPL( host_socket, gate_socket,
			     session_name, session_name_length,
			     password, password_length )) {}
Multiplexer::~Multiplexer() { delete impl; }


MultiplexerIMPL::MultiplexerIMPL(int host_socket, int gate_socket,
	   const char* session_name, size_t session_name_length,
	   const char* password, size_t password_length ) :
		host(host_socket), gate(gate_socket),
		num_guest(0),
		m_session_name_length(session_name_length),
		m_password_length(password_length)
{
	// FIXME MAX_SESSION_NAME_LENGTH > session_name_length
	// FIXME MAX_PASSWORD_LENGTH > password_length

	std::memcpy(m_session_name, session_name, m_session_name_length);
	std::memcpy(m_password, password, m_password_length);

	if( fcntl(host, F_SETFL, O_NONBLOCK) < 0 ) { pexit("set host nonblock"); }
	if( fcntl(gate, F_SETFL, O_NONBLOCK) < 0 ) { pexit("set gate nonblock"); }
	if( mpev.add(host, mp::EV_READ) < 0 ) { pexit("mpev.add host"); }
	if( mpev.add(gate, mp::EV_READ) < 0 ) { pexit("mpev.add gate"); }
}

MultiplexerIMPL::~MultiplexerIMPL()
{
	if( num_guest > 0 ) {
		int n = 0;
		int to = num_guest;
		for(int fd = 0; fd < INT_MAX; ++fd) {
			if( guest_set.test(fd) ) {
				close(fd);
				++n;
				if( n >= to ) { break; }
			}
		}
	}
}


int Multiplexer::run(void) { return impl->run(); }
int MultiplexerIMPL::run(void)
{
	int fd;
	short event;
	while(1) {
		while( mpev.next(&fd, &event) ) {
			if( fd == host ) {
				if( io_host(fd, event) < 0 ) { return 0; }
			} else if( fd == gate ) {
				if( io_gate(fd, event) < 0 ) { return 0; }
			} else {
				if( io_guest(fd, event) < 0 ) { return 0; }
			}
		}
		if( mpev.wait() < 0 ) {
			return -1;
		}
	}
	return 0;
}

int MultiplexerIMPL::io_host(int fd, short event)
{
	ssize_t len = read(fd, shared_buffer, SHARED_BUFFER_SIZE);
	if( len < 0 ) {
		if( errno == EAGAIN || errno == EINTR ) {
			// do nothing
		} else {
			perror("read from host failed");
			return -1;
		}
	} else if( len == 0 ) {
		perror("read from host ends");
		return -1;
	}
	// len > 0
	if( num_guest == 0 ) { return 0; }
	int n = 0;
	int to = num_guest;  // forの中でnum_guestが変動するので、ここで保存しておく
	for(int fd = 0; fd < INT_MAX; ++fd) {
		if( guest_set.test(fd) ) {
			send_to_guest(fd, shared_buffer, len);  // ここでremove_guest()が実行されるかもしれない
			++n;
			if( n >= to ) { break; }
		}
	}
	return 0;
}

int MultiplexerIMPL::io_gate(int fd, short event)
{
	gate_message_t msg;
	int guest = recvfd(gate, &msg, sizeof(msg));
	if( guest < 0 ) {
		if( errno == EAGAIN || errno == EINTR ) {
			return 0;
		} else {
			perror("guest connection failed a");
			return 0;
		}
	}
	if( msg.password.len != m_password_length ||
	    memcmp(msg.password.str, m_password, m_password_length) != 0 ||
	    msg.session_name.len != m_session_name_length ||
	    memcmp(msg.session_name.str, m_session_name, m_session_name_length) != 0 ) {
		perror("guest authentication failed");
		close(guest);
		return 0;
	}
	if( fcntl(guest, F_SETFL, O_NONBLOCK) < 0 ||
			mpev.add(guest, mp::EV_READ | mp::EV_WRITE) < 0 ) {
			// イベント待ちは readable or writable から始める
		perror("guest connection failed b");
		close(guest);
		return 0;
	}
	guest_set.set(guest, new filt_telnetd);
	guest_set.data(guest).send(PARTTY_SERVER_WELCOME_MESSAGE, strlen(PARTTY_SERVER_WELCOME_MESSAGE), NULL);
	num_guest++;
	return 0;
}

int MultiplexerIMPL::io_guest(int fd, short event)
{
	if( event & mp::EV_READ ) {
		ssize_t len = read(fd, shared_buffer, SHARED_BUFFER_SIZE);
		if( len < 0 ) {
			if( errno == EAGAIN || errno == EINTR ) {
				// do nothing
			} else {
				perror("read from guest failed");
				remove_guest(fd);
				return 0;
			}
		/* FIXME
		} else if( len == 0 ) {
			perror("read from guest ends");
			remove_guest(fd);
			return 0;
			*/
		} else {
			// guest -> host
			filt_telnetd::buffer_t ibuf;
			if( recv_filter(fd, shared_buffer, len, &ibuf) < 0 ) {
				// 切断されたゲストからのバッファは捨てる
				return 0;
			}
			// FIXME  write_all? ブロッキングモードにする？
			if( write_all(host, ibuf.buf, ibuf.len) != ibuf.len ) {
				perror("write to host failed");
				return -1;
			}
		}
	}
	if( event & mp::EV_WRITE ) {
		// 書き込み待ちバッファを書き込む
		filt_telnetd& srv( guest_set.data(fd) );
		ssize_t len = write(fd, srv.obuffer, srv.olength);
		if( srv.olength == 0 ) { return 0; }  // olengthは0の可能性がある
		if( len < 0 ) {
			if( errno == EAGAIN || errno == EINTR ) {
				// do nothing
			} else {
				perror("write to guest failed");
				remove_guest(fd);
				return 0;
			}
		} else if( len == 0 ) {
			perror("write to guest ends");
			remove_guest(fd);
			return 0;
		} else {
			if( (size_t)len < srv.olength ) {
				// 途中まで書き込めた
				srv.oconsumed(len);
			} else {
				// 全部書き込めた
				// writable待ちを解除する
				if( mpev.modify(fd, mp::EV_READ | mp::EV_WRITE, mp::EV_READ) < 0 ) {
					perror("mpev.modify failed");
					remove_guest(fd);
					return 0;
				}
				srv.oclear();
			}
		}
	}
	return 0;
}

int MultiplexerIMPL::recv_filter(int fd, const void* buf, size_t len, filt_telnetd::buffer_t* ibuf)
{
	filt_telnetd& srv( guest_set.data(fd) );
	bool may_writable = srv.is_oempty();
	srv.recv(buf, len, ibuf, NULL);  // recvしたときにもobufが発生する
	srv.iclear();   // 呼び出し側はibufを確実に使い切らないといけない
	if( may_writable ) {
		if( guest_try_write(fd, srv) < 0 ) {
			remove_guest(fd, srv);
			return -1;
		}
	}
	return 0;
}

int MultiplexerIMPL::send_to_guest(int fd, const void* buf, size_t len)
{
	filt_telnetd& srv( guest_set.data(fd) );
	bool may_writable = srv.is_oempty();
	srv.send(buf, len, NULL);
	if( may_writable ) {
		if( guest_try_write(fd, srv) < 0 ) {
			remove_guest(fd, srv);
			return -1;
		}
	}
	return 0;
}

int MultiplexerIMPL::guest_try_write(int fd, filt_telnetd& srv)
{
	// 現在の書き込み待ちバッファにあるバッファを書き込んでみて、
	// 全部書き込めたらそのまま、書き込めなかったら書き込み待ちにする
	// 書き込みバッファが無かった状態から書き込み待ちバッファがある状態に
	// 遷移したときにのみ、この関数を呼べる。
	if( srv.olength <= 0 ) { return 0; }
	ssize_t wlen = write(fd, srv.obuffer, srv.olength);
	if( wlen < 0 && errno != EAGAIN && errno != EINTR ) {
		perror("write to guest failed");
		return -1;
	} else if( wlen == 0 ) {
		perror("write to guest ends");
		return -1;
	}
	if( (size_t)wlen < srv.olength ) {
		// 全部書き込めなかった
		// バッファに残して書き込み待ちにする
		srv.oconsumed(wlen);
		if( mpev.modify(fd, mp::EV_READ, mp::EV_READ | mp::EV_WRITE) < 0 ) {
			return -1;
		}
	} else {
		// 全部書き込めた
		srv.oclear();
	}
	return 0;
}

void MultiplexerIMPL::remove_guest(int fd)
{
	filt_telnetd& srv( guest_set.data(fd) );
	remove_guest(fd, srv);
}

void MultiplexerIMPL::remove_guest(int fd, filt_telnetd& srv)
{
	mpev.remove(fd, mp::EV_READ | (srv.is_oempty() ? 0 : mp::EV_WRITE));
	guest_set.reset(fd);
	num_guest--;
	close(fd);
}


}  // namespace Partty

