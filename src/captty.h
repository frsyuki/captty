/*
 * Captty
 *
 * Copyright (C) 2007-2008 FURUHASHI Sadayuki, TOMATSURI Kaname
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

#ifndef CAPTTY_H__
#define CAPTTY_H__

#include <iostream>
#include <stdexcept>
#include <stdint.h>

namespace Captty {


void record(const char* file, char* cmd[] = NULL);
void play(const char* file);


class RecorderIMPL;
class Recorder {
public:
	Recorder(std::ostream& output);
	~Recorder();
	void write(const char* buf, uint16_t len);
	void set_window_size(short row, short col);
private:
	RecorderIMPL* impl;
	Recorder();
	Recorder(const Recorder&);
};


class PlayerIMPL;
class Player {
public:
	Player(std::istream& input);
	~Player();
	void play();
	void set_handler(void (*handler)(int, void*), void* data);
	void speed_up();
	void speed_down();
	void speed_reset();
	void speed_set(double speed);
	void pause();
	void toggle_pause();
	void skip_back();
	void skip_forward();
	void rewind();
	void quit();
private:
	PlayerIMPL* impl;
	Player();
	Player(const Player&);
};


struct captty_error : public std::runtime_error {
	captty_error(const std::string& msg) :
		std::runtime_error(msg) {}
	captty_error(const std::string& msg, int err) :
		std::runtime_error(msg) {}
};

struct io_error : captty_error {
	io_error(const std::string& msg) :
		captty_error(msg) {}
	io_error(const std::string& msg, int err) :
		captty_error(msg) {}
};


static const size_t MAX_BLOCK_SIZE = 1<<19;

struct tty_size {
	uint16_t row;
	uint16_t col;
};

namespace FRAME {
	static const uint8_t OUTPUT = 0;
	static const uint8_t WINDOW_SIZE = 1;
	static const size_t HEADER_SIZE = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t);
}

namespace BLOCK {
	static const uint8_t UNCOMPRESSED_FRAMES = 0;
	static const uint8_t COMPRESSED_FRAMES = 1;
	static const size_t HEADER_SIZE = sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint16_t);
}


inline uint16_t htoles(uint16_t x) {
#ifdef __BIG_ENDIAN__
	return ((x << 8) & 0xff00) | ((x >> 8) & 0x00ff);
#else
	return x;
#endif
}

inline uint32_t htolel(uint32_t x) {
#ifdef __BIG_ENDIAN__
	return  ((x << 24) & 0xff000000 ) |
		((x <<  8) & 0x00ff0000 ) |
		((x >>  8) & 0x0000ff00 ) |
		((x >> 24) & 0x000000ff );
#else
	return x;
#endif
}

inline uint16_t letohs(uint16_t x) { return htoles(x); }
inline uint32_t letohl(uint32_t x) { return htolel(x); }



/**
 * File format
 * ===========
 * +----+----+----+
 * |    |    |    | ...
 * +----+----+----+
 * Block
 *      Block
 *           Block
 *                ...
 *
 *
 * Block format
 * ============
 * +--+-+----+--------+
 * |16|8| 32 |        |
 * +--+-+----+--------+
 * time
 *    flag
 *      length
 *           frames
 *
 * time:    elapsed time since previous block
 *          in seconds
 *
 * flag:    BLOCK::COMPRESSED_FRAMES:
 *            following data is compressed frames
 *            compression algorithm is deflate
 *
 *          BLOCK::UNCOMPRESSED_FRAMES:
 *            following data is uncompressed frames
 *
 * length:  length of following data in bytes
 *          maximum is MAX_BLOCK_SIZE
 *
 *
 * Frames format
 * =============
 * +----+----+----+
 * |    |    |    | ...
 * +----+----+----+
 * Frame
 *      Frame
 *           Frame
 *                ...
 *
 *
 * Frame format
 * ============
 * +----+-+--+--------+
 * | 32 |8|16|        |
 * +----+-+--+--------+
 * time
 *      flag
 *        length
 *           data
 *
 * time:    elapsed time since previous frame
 *          in microseconds
 *
 * flag:    FRAME::OUTPUT
 *            following data is Terminal Output
 *
 *          FRAME::WINDOW_SIZE
 *            following data is Window Size
 *
 * length:  length of following data in bytes
 *          maximum is 1<<16
 *
 *
 * Terminal Output Format
 * ======================
 * +--------+
 * |        |
 * +--------+
 * data
 *
 *
 * Window Size Format
 * ==================
 * +--+--+
 * |16|16|
 * +--+--+
 * number of rows
 *    number of columns
 *
 */


}  // namespace Captty

#endif /* captty.h */
