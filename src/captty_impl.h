/*
 * This file is part of Captty.
 *
 * Copyright (C) 2008-2009 FURUHASHI Sadayuki
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CAPTTY_IMPL_H__
#define CAPTTY_IMPL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <vector>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "captty.h"

namespace Captty {


class BlockWriter {
public:
	BlockWriter(std::ostream& file);
	~BlockWriter();
public:
	void append_frame(uint32_t tdiff, const char* data,
			uint16_t data_length, uint8_t flag);
private:
	std::ostream& m_file;
	struct timeval tv_before;
	char* m_buffer;
	char* m_zbuffer;
	char* m_pos;
	static const time_t BLOCK_TIME_SPAN = 30;  //seconds
private:
	void reserve(size_t length);
	inline void bufcpy(const char* buf, size_t length);
	void write_block(uint16_t btdiff);
	void write_block_data(uint8_t flag, uint16_t btdiff, const char* data, uint32_t length);
	static inline time_t tv_diff(struct timeval& now, struct timeval& before);
private:
	BlockWriter();
	BlockWriter(const BlockWriter&);
};


inline void get_window_size(int fd, struct winsize* ws)
{
	if( ioctl(fd, TIOCGWINSZ, ws) < 0 ) {
		throw captty_error("can't get window size", errno);
	}
}

inline void set_window_size(int fd, struct winsize* ws)
{
	if( ioctl(fd, TIOCSWINSZ, ws) < 0 ) {
		throw captty_error("can't set window size", errno);
	}
}

inline bool cmp_window_size(struct winsize& wsr, struct winsize& wsl)
{
	return (wsr.ws_row == wsl.ws_row) && (wsr.ws_col == wsl.ws_col);
}


inline void get_time(struct timeval* tv)
{
	if( gettimeofday(tv, NULL) < 0 ) {
		throw captty_error("gettimeofday", errno);
	}
}


class RecorderIMPL {
public:
	RecorderIMPL(std::ostream& output);
	~RecorderIMPL();
	void write(const char* buf, uint16_t len);
	void set_window_size(short row, short col);
private:
	BlockWriter block_writer;
	struct timeval tv_before;
private:
	uint32_t tv_forward(struct timeval& next);
};


struct index_info_t {
	uint16_t btdiff;
	size_t seekpos;
};

class PlayerIMPL {
public:
	PlayerIMPL(std::istream& input);
	~PlayerIMPL();
	void play();
	inline void set_handler(void (*handler)(int, void*), void* data);
	inline void speed_up();
	inline void speed_down();
	inline void speed_reset();
	inline void speed_set(double speed);
	inline void skip_back();
	inline void skip_forward();
	inline void pause();
	inline void toggle_pause();
	inline void rewind();
	inline void quit();
private:
	std::istream& m_file;
	size_t m_block_pos;
	std::vector<index_info_t> m_index;
	double m_speed;
	bool m_skip_mode;
	bool m_skip_block;
	bool m_rewait_frame;
	bool m_quit;
	void (*m_handler)(int, void*);
	void* m_handler_data;
private:
	static void clear_display();
	bool play_block(const char* pos, const char* const endpos);
	inline void call_handler(int c);
	inline void default_handler(int c);
};



}  // namespace Captty

#endif /* catty_impl.h */
