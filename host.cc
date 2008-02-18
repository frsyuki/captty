#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "host.h"
#include "scoped_make_raw.h"
#include "ptyshell.h"
#include "unio.h"

#include <iostream>

namespace Partty {


// FIXME 例外処理

Host::Host(int server_socket,
	const char* session_name, size_t session_name_length,
	const char* password, size_t password_length ) :
		impl(new HostIMPL(server_socket,
			session_name, session_name_length,
			password, password_length )) {}

HostIMPL::HostIMPL(int server_socket,
	const char* session_name, size_t session_name_length,
	const char* password, size_t password_length ) :
		server(server_socket),
		m_session_name_length(session_name_length),
		m_password_length(password_length)
{
	// FIXME MAX_SESSION_NAME_LENGTH > session_name_length
	// FIXME MAX_PASSWORD_LENGTH > password_length

	std::memcpy(m_session_name, session_name, m_session_name_length);
	std::memcpy(m_password, password, m_password_length);
}


Host::~Host() { delete impl; }
HostIMPL::~HostIMPL() {}


int Host::run(void) { return impl->run(); }
int HostIMPL::run(void)
{
	// Serverにヘッダを送る
	negotiation_header_t header;
	memcpy(header.magic, NEGOTIATION_MAGIC_STRING, NEGOTIATION_MAGIC_STRING_LENGTH);

	// headerにはネットワークバイトオーダーで入れる
	header.user_name_length    = htons(0);   // user_nameは今のところ空
	header.session_name_length = htons(m_session_name_length);
	header.password_length     = htons(m_password_length);
	if( write_all(server, (char*)&header, sizeof(header)) != sizeof(header) ) {
		pexit("write header");
	}
	if( write_all(server, m_session_name, m_session_name_length) != m_session_name_length ) {
		pexit("write session");
	}
	if( write_all(server, m_password, m_password_length) != m_password_length ) {
		pexit("write password");
	}

	// 新しい仮想端末を確保して、シェルを起動する
	ptyshell psh(STDIN_FILENO);
	if( psh.fork(NULL) < 0 ) { pexit("psh.fork"); }
	sh = psh.masterfd();

	// 標準入力をRawモードにする
	// makerawはPtyChildShellをforkした後
	// （forkする前にRawモードにすると子仮想端末までRawモードになってしまう）
	PtyScopedMakeRaw makeraw(STDIN_FILENO);

	// 監視対象のファイルディスクリプタにO_NONBLOCKをセット
	if( fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) < 0 ) { pexit("set stdin nonblock");  }
	if( fcntl(server, F_SETFL, O_NONBLOCK) < 0 ) { pexit("set server nonblock"); }
	if( fcntl(sh, F_SETFL, O_NONBLOCK) < 0 ) { pexit("set sh nonblock");     }

	// 入力待ち
	using namespace mp::placeholders;
	if( mpdp.add(STDIN_FILENO, mp::EV_READ,
			mp::bind(&HostIMPL::io_stdin, this, _1, _2)) < 0 ) {
		pexit("mpdp.add stdin");
	}
	if( mpdp.add(server, mp::EV_READ,
			mp::bind(&HostIMPL::io_server, this, _1, _2)) < 0 ) {
		pexit("mpdp.add server");
	}
	if( mpdp.add(sh, mp::EV_READ,
			mp::bind(&HostIMPL::io_shell, this, _1, _2)) < 0 ) {
		pexit("mpdp.add sh");
	}

	// 端末のウィンドウサイズを取得しておく
	get_window_size(STDIN_FILENO, &winsz);

	// mp::dispatch::run
	return mpdp.run();
}


int HostIMPL::io_stdin(int fd, short event)
{
	// 標準入力 -> シェル
	ssize_t len = read(fd, shared_buffer, SHARED_BUFFER_SIZE);
	if( len < 0 ) {
		if( errno == EAGAIN || errno == EINTR ) {
			return 0;
		} else {
			pexit("read from stdin");
		}
	} else if( len == 0 ) {
		perror("end of stdin");
	}
	if( write_all(sh, shared_buffer, len) != (size_t)len ){
		pexit("write to sh");
	}
	// 標準入力のウィンドウサイズが変更されたら子仮想端末にも反映する
	struct winsize next;
	get_window_size(fd, &next);
	if( winsz.ws_row != next.ws_row || winsz.ws_col != next.ws_col ) {
		set_window_size(fd, &next);
		winsz = next;
	}
	return 0;
}

int HostIMPL::io_server(int fd, short event)
{
	// Server -> シェル
	ssize_t len = read(fd, shared_buffer, SHARED_BUFFER_SIZE);
	if( len < 0 ) {
		if( errno == EAGAIN || errno == EINTR ) {
			return 0;
		} else {
			pexit("read from server");
		}
	} else if( len == 0 ) {
		perror("end of server");
		return 1;
	}
	if( write_all(sh, shared_buffer, len) != (size_t)len ){
		pexit("write to sh");
	}
	return 0;
}

int HostIMPL::io_shell(int fd, short event)
{
	// シェル -> 標準出力
	// シェル -> Server
	ssize_t len = read(fd, shared_buffer, SHARED_BUFFER_SIZE);
	if( len < 0 ) {
		if( errno == EAGAIN || errno == EINTR ) {
			return 0;
		} else {
			pexit("read from sh");
		}
	} else if( len == 0 ) {
		perror("end of sh");
		return 1;
	}
	if( write_all(STDOUT_FILENO, shared_buffer, len) != (size_t)len ) {
		pexit("write to stdout");
	}
	if( write_all(server, shared_buffer, len) != (size_t)len ) {
		pexit("write to server");
	}
	// 標準出力の転送が終わるまで待つ
	// （転送スピードを超過して書き込むと端末が壊れるため）
	tcdrain(STDOUT_FILENO);
	return 0;
}

int HostIMPL::get_window_size(int fd, struct winsize* ws)
{
	return ioctl(fd, TIOCGWINSZ, ws);
}

int HostIMPL::set_window_size(int fd, struct winsize* ws)
{
	return ioctl(fd, TIOCSWINSZ, ws);
}


}  // namespace Partty

