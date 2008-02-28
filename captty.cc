#include <cstring>
#include <zlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <fstream>
#include <algorithm>
#include <limits>
#include "captty.h"
#include "captty_impl.h"

#ifdef HAVE_UTIL_H
#include <util.h>
#endif
#ifdef HAVE_PTY_H
#include <pty.h>
#endif

namespace Captty {


namespace {

int execute_shell(char* cmd[])
{
	if( cmd == NULL ) {
		char* shell = getenv("SHELL");
		char fallback_shell[] = "/bin/sh";
		if( shell == NULL ) { shell = fallback_shell; }
		char* name = strrchr(shell, '/');
		if( name == NULL ) { name = shell; } else { name += 1; }
		execl(shell, name, "-i", NULL);
		perror(shell);
		return 127;
	} else {
		execvp(cmd[0], cmd);
		perror(cmd[0]);
		return 127;
	}
}

inline size_t write_all(int fd, const char *buf, size_t count) {
	const char *p = buf;
	const char * const endp = p + count;
	do {
		const int num_bytes = write(fd, p, endp - p);
		if( num_bytes < 0 ) {
			if( errno != EINTR && errno != EAGAIN ) {
				throw io_error("output", errno);
			}
		} else {
			p += num_bytes;
		}
	} while (p < endp);
	return count;
}

class scoped_pty_raw {
public:
	scoped_pty_raw(int fd) : m_fd(fd) {
		tcgetattr(m_fd, &m_orig);
		struct termios raw = m_orig;
		cfmakeraw(&raw);
		//raw.c_lflag &= ~ECHO;
		tcsetattr(m_fd, TCSAFLUSH, &raw);
	}
	~scoped_pty_raw() {
		finish();
	}
	void finish(void) {
		if(m_fd >= 0) {
			// XXX: TCSANOW?
			tcsetattr(m_fd, TCSADRAIN, &m_orig);
			write(m_fd, "\n", 1);
			m_fd = -1;
		}
	}
private:
	int m_fd;
	struct termios m_orig;
private:
	scoped_pty_raw();
	scoped_pty_raw(const scoped_pty_raw&);
};

class scoped_window_save {
public:
	scoped_window_save(int fd) : m_fd(fd) {
		get_window_size(m_fd, &m_ws);
	}
	~scoped_window_save() try {
		set_window_size(m_fd, &m_ws);
	} catch (...) {
	}
private:
	int m_fd;
	struct winsize m_ws;
private:
	scoped_window_save();
	scoped_window_save(const scoped_window_save&);
};

}  // noname namespace


BlockWriter::BlockWriter(std::ostream& file) :
		m_file(file)
{
	m_buffer = new char[MAX_BLOCK_SIZE];
	m_pos = m_buffer;
	try {
		m_zbuffer = new char[MAX_BLOCK_SIZE];
	} catch (...) {
		delete[] m_buffer;
		throw std::bad_alloc();
	}
	get_time(&tv_before);
}

BlockWriter::~BlockWriter()
{
	struct timeval tv_now;
	get_time(&tv_now);
	time_t btdiff = tv_diff(tv_now, tv_before);

	try {
		write_block((uint16_t)btdiff);
	} catch (io_error& e) {
		perror(e.what());
	} catch (...) {
		perror("write_block");
	}

	delete[] m_buffer;
	delete[] m_zbuffer;
}


void BlockWriter::reserve(size_t length)
{
	struct timeval tv_now;
	get_time(&tv_now);
	time_t btdiff = tv_diff(tv_now, tv_before);

	if( btdiff > BLOCK_TIME_SPAN ) {
		tv_before = tv_now;
		write_block((uint16_t)btdiff);
	} else if( MAX_BLOCK_SIZE - (m_pos - m_buffer) < length ) {
		tv_before = tv_now;
		write_block((uint16_t)btdiff);
	}
}

void BlockWriter::append_frame(uint32_t tdiff, const char* data,
		uint16_t data_length, uint8_t flag)
{
	reserve(FRAME::HEADER_SIZE + data_length);
	uint32_t tdiff_le = htolel(tdiff);
	uint16_t data_length_le = htoles(data_length);
	bufcpy((const char*)&tdiff_le, sizeof(uint32_t));
	bufcpy((const char*)&flag, sizeof(uint8_t));
	bufcpy((const char*)&data_length_le, sizeof(uint16_t));
	bufcpy((const char*)data, data_length);
}

void BlockWriter::bufcpy(const char* buf, size_t length)
{
	std::memcpy(m_pos, buf, length);
	m_pos += length;
}

void BlockWriter::write_block(uint16_t btdiff)
{
	uint32_t length = m_pos - m_buffer;
	size_t zlength = MAX_BLOCK_SIZE;
	if( compress( (Bytef*)m_zbuffer,(uLongf*)&zlength, (Bytef*)m_buffer, length) == Z_OK &&
			zlength < length ) {
		write_block_data(BLOCK::COMPRESSED_FRAMES, btdiff,
				m_zbuffer, zlength);
	} else {
		write_block_data(BLOCK::UNCOMPRESSED_FRAMES, btdiff,
				m_buffer, length);
	}
	m_pos = m_buffer;
}

void BlockWriter::write_block_data(uint8_t flag, uint16_t btdiff,
		const char* data, uint32_t length)
{
	uint16_t btdiff_le = htoles(btdiff);
	uint32_t length_le = htolel(length);
	if( !m_file.write((char*)&btdiff_le,sizeof(uint16_t)).good() ) {
		throw io_error("write file btdiff");
	}
	if( !m_file.write((char*)&flag,sizeof(uint8_t)).good() ) {
		throw io_error("write file flag");
	}
	if( !m_file.write((char*)&length_le,sizeof(uint32_t)).good() ) {
		throw io_error("write file length");
	}
	if( !m_file.write((char*)data,length).good() ) {
		throw io_error("write file buffer");
	}
}

time_t BlockWriter::tv_diff(struct timeval& now, struct timeval& before) {
	return (now.tv_sec - before.tv_sec);
}



Recorder::Recorder(std::ostream& output) :
	impl(new RecorderIMPL(output)) {}

RecorderIMPL::RecorderIMPL(std::ostream& output) :
	block_writer(output)
{
	get_time(&tv_before);
}

Recorder::~Recorder() { delete impl; }
RecorderIMPL::~RecorderIMPL() {}


uint32_t RecorderIMPL::tv_forward(struct timeval& next)
{
	uint32_t diff = (next.tv_sec - tv_before.tv_sec)*1000*1000
		+ (next.tv_usec - tv_before.tv_usec);
	tv_before = next;
	return diff;
}

void Recorder::write(const char* buf, uint16_t len) { impl->write(buf, len); }
void RecorderIMPL::write(const char* buf, uint16_t len)
{
	struct timeval tv_now;
	get_time(&tv_now);
	block_writer.append_frame(
			tv_forward(tv_now),
			buf, len,
			FRAME::OUTPUT
			);
}

void Recorder::set_window_size(short row, short col) { impl->set_window_size(row, col); }
void RecorderIMPL::set_window_size(short row, short col)
{
	struct timeval tv_now;
	get_time(&tv_now);
	tty_size tsz = {row, col};
	block_writer.append_frame(
			tv_forward(tv_now),
			(char*)&tsz, sizeof(tsz),
			FRAME::WINDOW_SIZE
			);
}

void record(const char* file, char* cmd[])
{
	std::ofstream output(file, std::ios::binary | std::ios::trunc);

	// termios, winsizeの引き継ぎ
	struct termios tios;
	tcgetattr(STDIN_FILENO, &tios);

	struct winsize ws_before;
	get_window_size(STDIN_FILENO, &ws_before);

	int master;
	int slave;
	if( openpty(&master, &slave, NULL, &tios, &ws_before) == -1 ) {
		throw captty_error("can't open pty", errno);
	}

	// fork
	pid_t pid = fork();
	if( pid < 0 ) {
		close(master);
		close(slave);
		throw captty_error("can't fork", errno);
	} else if( pid == 0 ) {
		// child
		close(master);
		setsid();
		ioctl(slave, TIOCSCTTY, 0);
		//dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		close(slave);
		exit( execute_shell(cmd) );
	}
	// parent
	close(slave);

	RecorderIMPL recorder(output);

	ws_before.ws_row = 0;
	ws_before.ws_col = 0;
	struct winsize ws_now;

	ssize_t len;
	char buf[1<<16];
	while( (len = read(master, buf, sizeof(buf))) > 0 ) {
		get_window_size(STDIN_FILENO, &ws_now);
		if( !cmp_window_size(ws_before, ws_now) ) {
			set_window_size(master, &ws_now);
			recorder.set_window_size(ws_now.ws_row, ws_now.ws_col);
			ws_before = ws_now;
		}
		write_all(STDOUT_FILENO, buf, len);
		recorder.write(buf, len);
	}
}


PlayerIMPL::PlayerIMPL(std::istream& input) :
	m_file(input),
	m_block_pos(0),
	m_speed(1.0),
	m_skip_mode(false),
	m_skip_block(false),
	m_rewait_frame(false),
	m_quit(false),
	m_handler(NULL),
	m_handler_data(NULL)
{
	char buf[BLOCK::HEADER_SIZE];
	while( m_file.read(buf, BLOCK::HEADER_SIZE).good() ) {
		uint16_t btdiff = letohs( *(uint16_t*)buf );
		//uint8_t& bflag( *(uint8_t*)(buf + sizeof(uint16_t)) );
		uint32_t blength = letohl( *(uint32_t*)(buf + sizeof(uint16_t) + sizeof(uint8_t)) );
		size_t fp = (size_t)m_file.tellg() - BLOCK::HEADER_SIZE;
		index_info_t info;
		info.btdiff = btdiff;
		info.seekpos = fp;
		m_index.push_back(info);
		m_file.seekg(blength, std::ios::cur);
	}
	m_file.clear();
	m_file.seekg(0, std::ios::beg);
}

Player::~Player() { delete impl; }
PlayerIMPL::~PlayerIMPL() {}

void Player::play() { impl->play(); }
void PlayerIMPL::play()
{
	scoped_pty_raw scoped_raw(STDIN_FILENO);
	if( fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) < 0 ) {
		throw io_error("fcntl O_NONBLOCK", errno);
	}

	scoped_window_save scoped_ws(STDIN_FILENO);

	char buf[MAX_BLOCK_SIZE + BLOCK::HEADER_SIZE];
	char zbuf[MAX_BLOCK_SIZE];
	size_t zlen;
	while( m_file.read(buf, BLOCK::HEADER_SIZE).good() && !m_quit ) {
		//uint16_t btdiff = letohs( *(uint16_t*)buf );
		uint8_t& bflag( *(uint8_t*)(buf + sizeof(uint16_t)) );
		uint32_t blength = letohl( *(uint32_t*)(buf + sizeof(uint8_t) + sizeof(uint16_t)) );
		if( blength > MAX_BLOCK_SIZE ) {
			throw io_error("invalid block length");
		}
		if( !m_file.read(buf+BLOCK::HEADER_SIZE, blength).good() ) {
			throw io_error("read block");
		}
		m_block_pos++;

		const char* pos;
		const char* endpos;
		switch(bflag) {
		case BLOCK::COMPRESSED_FRAMES:
			zlen = sizeof(zbuf);
			if( uncompress((Bytef*)zbuf, (uLongf*)&zlen,
						(Bytef*)(buf+BLOCK::HEADER_SIZE), blength) != Z_OK ) {
				throw io_error("uncompress error");
			}
			pos = zbuf;
			endpos = zbuf + zlen;	
			break;
		case BLOCK::UNCOMPRESSED_FRAMES:
			pos = buf + BLOCK::HEADER_SIZE;
			endpos = buf + BLOCK::HEADER_SIZE + blength;
			break;
		default:  // unsupported block
			continue;  // skip this block
		}

		if( !play_block(pos, endpos) ) {
			m_skip_mode = false;
		}
	}
	if( !m_file.good() && !m_file.eof() ) {
		throw io_error("read file exit", errno);
	}
}


bool PlayerIMPL::play_block(const char* pos, const char* const endpos)
{
	while(pos < endpos && !m_quit) {
		uint32_t ftdiff = letohl( *(uint32_t*)pos );
		uint8_t& fflag( *(uint8_t*)(pos + sizeof(uint32_t)) );
		uint16_t fdata_length = letohs( *(uint16_t*)(pos + sizeof(uint32_t) + sizeof(uint8_t)) );

		struct timeval tv;
		if( m_skip_mode ) {
			tv.tv_sec  = 0;
			tv.tv_usec = 0;
		} else {
			double d = ftdiff / m_speed;
			ftdiff = static_cast<uint32_t>( std::min( d, static_cast<double>(std::numeric_limits<uint32_t>::max()) ) );
			tv.tv_sec  = ftdiff / (1000 * 1000);
			tv.tv_usec = ftdiff % (1000 * 1000);
		}
		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(STDIN_FILENO, &read_set);
		int numev = select(STDIN_FILENO+1, &read_set, NULL, NULL, &tv);

		if( numev > 0 ) {
			char rbuf[512];
			ssize_t rlen;
			if( (rlen = read(STDIN_FILENO, rbuf, sizeof(rbuf))) < 0 ) {
				if( errno == EAGAIN || errno == EINTR ) {
					// do nothing
				} else {
					throw io_error("read stdinput", errno);
				}
			} else if( rlen == 0 ) {
				throw io_error("read stdinput ends");
			} else {
				call_handler(rbuf[0]);
				if( m_skip_block ) {
					m_skip_block = false;
					return true;
				} else if( m_rewait_frame ) {
					m_rewait_frame = false;
					continue;
				}
			}
		}

		switch(fflag) {
		case FRAME::OUTPUT:
			write_all(STDOUT_FILENO, pos+FRAME::HEADER_SIZE, fdata_length);
			break;
		case FRAME::WINDOW_SIZE: {
			tty_size& tsz( *(tty_size*)(pos+FRAME::HEADER_SIZE) );
			struct winsize ws = {tsz.row, tsz.col};
			set_window_size(STDOUT_FILENO, &ws);
			} break;
		}

		pos += FRAME::HEADER_SIZE + fdata_length;
	}
	return false;
}

void Player::set_handler(void (*handler)(int, void*), void* data)
	{ impl->set_handler(handler, data); }
void PlayerIMPL::set_handler(void (*handler)(int, void*), void* data)
{
	m_handler = handler;
	m_handler_data = data;
}

void Player::speed_up() { impl->speed_up(); }
void PlayerIMPL::speed_up()
{
	if( m_speed < m_speed*2 ) {
		m_speed *= 2;
	}
}

void Player::speed_down() { impl->speed_down(); }
void PlayerIMPL::speed_down()
{
	if( m_speed / 2 != 0 ) {
		m_speed /= 2;
	}
}

void Player::speed_reset() { impl->speed_reset(); }
void PlayerIMPL::speed_reset()
{
	m_speed = 1.0;
}

void Player::speed_set(double speed) { impl->speed_set(speed); }
void PlayerIMPL::speed_set(double speed)
{
	m_speed = speed;
}

void Player::skip_back() { impl->skip_back(); }
void PlayerIMPL::skip_back()
{
	if( m_block_pos < 3 ) {
		m_block_pos = 0;
	} else {
		m_block_pos -= 3;
		m_skip_mode = true;
	}
	m_file.seekg(m_index[m_block_pos].seekpos, std::ios::beg);
	m_skip_block = true;
	clear_display();
}

void Player::skip_forward() { impl->skip_forward(); }
void PlayerIMPL::skip_forward()
{
	m_skip_mode = true;
}

void Player::pause() { impl->pause(); }
void PlayerIMPL::pause()
{
	m_speed = std::numeric_limits<double>::min();
	m_rewait_frame = true;
}

void Player::toggle_pause() { impl->toggle_pause(); }
void PlayerIMPL::toggle_pause()
{
	if( m_speed == std::numeric_limits<double>::min() ) {
		m_speed = 1.0;
	} else {
		m_speed = std::numeric_limits<double>::min();
		m_rewait_frame = true;
	}
}

void Player::rewind() { impl->rewind(); }
void PlayerIMPL::rewind()
{
	m_file.seekg(m_index[0].seekpos, std::ios::beg);
	m_block_pos = 0;
	m_skip_block = true;
	clear_display();
}

void Player::quit() { impl->quit(); }
void PlayerIMPL::quit()
{
	m_quit = true;
	m_skip_block = true;
	clear_display();
}

void PlayerIMPL::clear_display()
{
	static const char sequence[] = {0x1b, 0x5b, 0x48, 0x1b, 0x5b, 0x32, 0x4a};
	std::cout.write(sequence, sizeof(sequence));
	std::cout << std::flush;
}

void PlayerIMPL::call_handler(int c)
{
	if(m_handler) {
		(*m_handler)(c, m_handler_data);
	} else {
		default_handler(c);
	}
}

void PlayerIMPL::default_handler(int c)
{
	switch(c) {
	case 'h':
		skip_back();
		break;
	case 'l':
		skip_forward();
		break;
	case 'j':
		speed_down();
		break;
	case 'k':
		speed_up();
		break;
	case 'g':
		rewind();
		break;
	case ';':
		toggle_pause();
		break;
	case '=':
		speed_reset();
		break;
	case 'q':
		quit();
		break;
	}
}

void play(const char* file)
{
	std::ifstream input(file, std::ios::binary);
	PlayerIMPL player(input);
	player.play();
}


}  // namespace Captty

