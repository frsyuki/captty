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


}  // namespace Captty

#endif /* captty.h */
