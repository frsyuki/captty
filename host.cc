#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "host.h"
#include "pty_make_raw.h"
#include "ptyshell.h"
#include "uniext.h"

#include <iostream>

namespace Partty {


// FIXME Serverとのコネクションが切断したら再接続する？

Host::Host(int server_socket, char lock_code,
		const session_info_ref_t& info) :
		impl( new HostIMPL(server_socket, lock_code,
					info) ) {}

HostIMPL::HostIMPL(int server_socket, char lock_code,
		const session_info_ref_t& info) :
	server(server_socket),
	m_lock_code(lock_code), m_locking(false),
	m_info(info) {}


Host::~Host() { delete impl; }
HostIMPL::~HostIMPL() {}


int Host::run(void) { return impl->run(); }
int HostIMPL::run(void)
{
	// Serverにヘッダを送る
	negotiation_header_t header;

	// negotiation magic string
	memcpy(header.magic, NEGOTIATION_MAGIC_STRING, NEGOTIATION_MAGIC_STRING_LENGTH);
	header.protocol_version = PROTOCOL_VERSION;

	// headerにはネットワークバイトオーダーで入れる
	header.user_name_length    = htons(0);   // user_nameは今のところ空
	header.session_name_length = htons(m_info.session_name_length);
	header.writable_password_length = htons(m_info.writable_password_length);
	header.readonly_password_length = htons(m_info.readonly_password_length);
	// header
	if( write_all(server, &header, sizeof(header)) != sizeof(header) ) {
		throw initialize_error("failed to send negotiation header");
	}
	// user_name
	if( write_all(server, &m_info.user_name, m_info.user_name_length) != m_info.user_name_length ) {
		throw initialize_error("failed to send user name");
	}
	// session_name
	if( write_all(server, m_info.session_name, m_info.session_name_length) != m_info.session_name_length ) {
		throw initialize_error("failed to send session name");
	}
	// writable_password
	if( write_all(server, m_info.writable_password, m_info.writable_password_length) != m_info.writable_password_length ) {
		throw initialize_error("failed to send operation password");
	}
	// readonly_password
	if( write_all(server, m_info.readonly_password, m_info.readonly_password_length) != m_info.readonly_password_length ) {
		throw initialize_error("failed to send view-only password");
	}

	// Serverからヘッダを受け取る
	negotiation_reply_t reply;
	if( read_all(server, &reply, sizeof(reply)) != sizeof(reply) ) {
		throw initialize_error("failed to receive negotiation reply");
	}

	// ヘッダをチェック
	{
		uint16_t msglen = ntohs(reply.message_length);
		char* msg = (char*)malloc(msglen+1);
		if( msg == NULL ) {
			throw initialize_error("failed to receive negotiation reply");
		}
		if( read_all(server, msg, msglen) != msglen ) {
			free(msg);
			throw initialize_error("failed to receive negotiation reply");
		}
		if( reply.code != negotiation_reply::SUCCESS ) {
			msg[msglen] = '\0';
			initialize_error e(msg);
			free(msg);
			throw e;
		}
	}

	// 新しい仮想端末を確保して、シェルを起動する
	ptyshell psh(STDIN_FILENO);
	if( psh.fork(NULL) < 0 ) { throw initialize_error("can't execute shell"); }
	sh = psh.masterfd();

	// 標準入力をRawモードにする
	// ptyrawはptyshellをforkした後
	// （forkする前にRawモードにすると子仮想端末までRawモードになってしまう）
	scoped_pty_make_raw ptyraw(STDIN_FILENO);

	// 監視対象のファイルディスクリプタにO_NONBLOCKをセット
	if( fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) < 0 ) {
		throw initialize_error("failed to set stdinput nonblocking mode");
	}
	if( fcntl(server, F_SETFL, O_NONBLOCK) < 0 ) {
		throw initialize_error("failed to set server socket nonblocking mode");
	}
	if( fcntl(sh, F_SETFL, O_NONBLOCK) < 0 ) {
		throw initialize_error("failed to set pty nonblocking mode");
	}

	// mp::dispatchに登録
	using namespace mp::placeholders;
	if( mpdp.add(STDIN_FILENO, mp::EV_READ,
			mp::bind(&HostIMPL::io_stdin, this, _1, _2)) < 0 ) {
		throw initialize_error("can't add stdinput to IO multiplexer");
	}
	if( mpdp.add(server, mp::EV_READ,
			mp::bind(&HostIMPL::io_server, this, _1, _2)) < 0 ) {
		throw initialize_error("can't add server socket to IO multiplexer");
	}
	if( mpdp.add(sh, mp::EV_READ,
			mp::bind(&HostIMPL::io_shell, this, _1, _2)) < 0 ) {
		throw initialize_error("can't add pty to IO multiplexer");
	}

	// 端末のウィンドウサイズを取得しておく
	get_window_size(STDIN_FILENO, &winsz);

	std::cout << SESSION_START_MESSAGE << std::flush;

	// mp::dispatch::run
	try {
		int ret = mpdp.run();
		if( ret != 0 ) {
			throw io_error("multiplexer broken");
		}
		throw std::logic_error("multiplexer error");
	} catch (io_end_error& e) {
		ptyraw.finish();
		std::cout << e.what() << std::endl;
		std::cout << SESSION_END_MESSAGE << std::endl;
	}
	return 0;
}


int HostIMPL::io_stdin(int fd, short event)
{
	// 標準入力 -> シェル
	ssize_t len = read(fd, shared_buffer, SHARED_BUFFER_SIZE);
	if( len < 0 ) {
		if( errno == EAGAIN || errno == EINTR ) { return 0; }
		else { throw io_error("stdinput is broken"); }
	} else if( len == 0 ) { throw io_end_error("end of stdinput"); }
	// ブロックしながらシェルに書き込む
	if( continued_blocking_write_all(sh, shared_buffer, len) != (size_t)len ){
		throw io_error("pty is broken");
	}
	// 標準入力のウィンドウサイズが変更されたら子仮想端末にも反映する
	struct winsize next;
	get_window_size(fd, &next);
	if( winsz.ws_row != next.ws_row || winsz.ws_col != next.ws_col ) {
		set_window_size(sh, &next);
		winsz = next;
	}
	// lock_codeが含まれていたらm_lockingをトグルする
	for(const char *p=shared_buffer, *p_end=p+len; p != p_end; ++p) {
		if(*p == m_lock_code) {
			m_locking = !m_locking;
		}
	}
	return 0;
}

int HostIMPL::io_server(int fd, short event)
{
	// Server -> シェル
	ssize_t len = read(fd, shared_buffer, SHARED_BUFFER_SIZE);
	if( len < 0 ) {
		if( errno == EAGAIN || errno == EINTR ) { return 0; }
		else { throw io_error("server connection is broken"); }
	} else if( len == 0 ) { throw io_end_error("server connection closed"); }
	// ロック中ならServerからの入力は捨てる
	if( m_locking ) { return 0; }
	// ブロックしながらシェルに書き込む
	if( continued_blocking_write_all(sh, shared_buffer, len) != (size_t)len ){
		throw io_error("pty is broken");
	}
	return 0;
}

int HostIMPL::io_shell(int fd, short event)
{
	// シェル -> 標準出力
	// シェル -> Server
	ssize_t len = read(fd, shared_buffer, SHARED_BUFFER_SIZE);
	if( len < 0 ) {
		if( errno == EAGAIN || errno == EINTR ) { return 0; }
		else { throw io_error("pty is broken"); }
	} else if( len == 0 ) { throw io_end_error("session ends"); }
	// ブロックしながら標準出力に書き込む
	if( continued_blocking_write_all(STDOUT_FILENO, shared_buffer, len) != (size_t)len ) {
		throw io_error("stdoutput is broken");
	}
	// ブロックしながらServerに書き込む
	if( continued_blocking_write_all(server, shared_buffer, len) != (size_t)len ) {
		throw io_error("server connection is broken");
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

