#include "emtelnet.h"
#include <cstring>


emtelnet::emtelnet(void* user_data) :
	olength(0), ilength(0),
	user(user_data),
	m_ostate_cr(false), m_istate(NULL)
{
	// FIXME 初期バッファのサイズは？
	if( (obuffer = (char*)malloc(BUFSIZ)) == NULL ) {
		throw std::bad_alloc();
	}
	oalloced = BUFSIZ;
	if( (ibuffer = (char*)malloc(BUFSIZ)) == NULL ) {
		free(obuffer);
		throw std::bad_alloc();
	}
	ialloced = BUFSIZ;
	std::memset(m_my_option_handler, 0, sizeof(m_my_option_handler));
	std::memset(m_partner_option_handler, 0, sizeof(m_partner_option_handler));
	std::memset(m_sb_handler, 0, sizeof(m_sb_handler));
}

emtelnet::~emtelnet()
{
	free(obuffer);
	free(ibuffer);
}

void emtelnet::send(const void* buf, size_t len)
{
	const byte* from = (const byte*)buf;
	const byte* p = from;
	const byte* endp = from + len;
	for(; p != endp; ++p) {
#ifdef EMTELNET_CONVERT_CRLF_LF
		// ensure that CR is never send alone but CRLF
		if( !m_ostate_cr && *p == LF ) {
			// CR is not sent but LF comes
			// send currnt buffer + CRLF, rebuffer
			owrite(from, p - from);
			owrite1(CR);
			// LF will be sent next
			from = p;
		} else if( m_ostate_cr ) {
			if( *p == LF ) {
				// CRLF is ok. pass it throgh
				m_ostate_cr = false;
			} else {
				// CR is sent but LF doesn't come
				// send current buffer, send LF, rebbuffer
				owrite(from, p - from);
				owrite1(LF);
				from = p;
				if( *p != CR ) {
					// leave m_ostate_cr flag true if *p == CR
					m_ostate_cr = false;
				}
			}
		} else if( *p == IAC ) {
			// ensure that IAC is never send alone but IAC IAC
			owrite(from, p - from);
			owrite1(IAC);
			from = p;
		} else if( *p == CR ) {
			m_ostate_cr = true;
		}
#else
		if( *p == IAC ) {
			// ensure that IAC is never send alone but IAC IAC
			owrite(from, p - from);
			owrite1(IAC);
			from = p;
		}
#endif
	}
	owrite(from, p - from);
}

void emtelnet::recv(const void* buf, size_t len)
{
	// needed ibuffer capacity is len * 2.
	// the worst case is all bytes are IAC.
	irealloc(ilength + len * 2);

	const byte* from = (const byte*)buf;
	const byte* p = from;
	const byte* endp = from + len;
	for(; p != endp; ++p) {
		if( m_istate == NULL ) {
			istate_NORMAL(*p);
		} else {
			(this->*m_istate)(*p);
		}
	}
}

void emtelnet::send_will(byte cmd)
{
	owrite3(IAC, WILL, cmd);
	m_do_waiting.set(cmd);
}

void emtelnet::send_wont(byte cmd)
{
	owrite3(IAC, WONT, cmd);
	m_do_waiting.set(cmd);
}

void emtelnet::send_do(byte cmd)
{
	owrite3(IAC, DO, cmd);
	m_will_waiting.set(cmd);
}

void emtelnet::send_dont(byte cmd)
{
	owrite3(IAC, DONT, cmd);
	m_will_waiting.set(cmd);
}

void emtelnet::send_sb(byte cmd, const void* msg, size_t len)
{
	owrite3(IAC, SB, cmd);
	owrite((const byte*)msg, len);
	owrite2(IAC, SE);
}

void emtelnet::owrite1(byte c1)
{
	orealloc(1);
	obuffer[olength++] = c1;
}

void emtelnet::owrite2(byte c1, byte c2)
{
	orealloc(2);
	obuffer[olength++] = c1;
	obuffer[olength++] = c2;
}

void emtelnet::owrite3(byte c1, byte c2, byte c3)
{
	orealloc(3);
	obuffer[olength++] = c1;
	obuffer[olength++] = c2;
	obuffer[olength++] = c3;
}

void emtelnet::owrite4(byte c1, byte c2, byte c3, byte c4)
{
	orealloc(4);
	obuffer[olength++] = c1;
	obuffer[olength++] = c2;
	obuffer[olength++] = c3;
	obuffer[olength++] = c4;
}


void emtelnet::owrite(const byte* s, size_t len)
{
	orealloc(len);
	memcpy(obuffer + olength, s, len);
	olength += len;
}

void emtelnet::orealloc(size_t reqlen)
{
	if( oalloced < olength + reqlen ) {
		size_t next_alloc = oalloced * 2 + reqlen;
		char* tmp = (char*)realloc(obuffer, next_alloc);
		if( tmp == NULL ) { throw std::bad_alloc(); }
		obuffer = tmp;
		oalloced = next_alloc;
	}
}


// all of needed ibuffer is allocated on the start of send().
void emtelnet::iwrite(byte c)
{
	ibuffer[ilength++] = c;
}

void emtelnet::iwrite(const byte* s, size_t len)
{
	memcpy(ibuffer + ilength, s, len);
	ilength += len;
}

void emtelnet::irealloc(size_t reqlen)
{
	if( ialloced < ilength + reqlen ) {
		size_t next_alloc = ialloced * 2 + reqlen;
		char* tmp = (char*)realloc(ibuffer, next_alloc);
		if( tmp == NULL ) { throw std::bad_alloc(); }
		ibuffer = tmp;
		ialloced = next_alloc;
	}
}


void emtelnet::istate_NORMAL(byte c)
{
#ifdef EMTELNET_CONVERT_CRLF_LF
	switch(c) {
	case CR:
		m_istate = &emtelnet::istate_CR;
		break;
	case IAC:
		m_istate = &emtelnet::istate_IAC;
		break;
	default:
		iwrite(c);
		break;
	}
#else
	if( c == IAC ) {
		m_istate = &emtelnet::istate_IAC;
	} else {
		iwrite(c);
	}
#endif
}

#ifdef EMTELNET_CONVERT_CRLF_LF
void emtelnet::istate_CR(byte c)
{
	if( c == LF ) {
		// CRLF comes, convert it to LF
		iwrite(LF);
	} else {
		// only CR comes, write LF and the non-CR byteacter
		iwrite(LF);
		iwrite(c);
	}
	m_istate = NULL;
}
#endif

void emtelnet::istate_IAC(byte c)
{
	switch(c) {
	case IAC:
		// IAC-escaped IAC. write one IAC
		iwrite(IAC);
		m_istate = NULL;
		break;
	case WILL:
		m_istate = &emtelnet::istate_WILL;
		break;
	case WONT:
		m_istate = &emtelnet::istate_WONT;
		break;
	case DO:
		m_istate = &emtelnet::istate_DO;
		break;
	case DONT:
		m_istate = &emtelnet::istate_DONT;
		break;
	case SB:
		// subnegotiation
		m_istate = &emtelnet::istate_SB;
		break;
	// FIXME there are many other telnet commands ...
	//       NO, DM, AYT, ...
	default:
		// broken or non-supported protocol
		iwrite(IAC);
		iwrite(c);
		m_istate = NULL;
		break;
	}
}

void emtelnet::istate_WILL(byte c)
{
	//fprintf(stderr, "will %u\n", c);
	if( m_will_waiting.test(c) ) {
		// do (or maybe dont) is sent and receive will
		// the partner can use the option
		m_will_waiting.reset(c);
		if( m_partner_option_handler[(size_t)c] ) {
			m_partner_option_handler[(size_t)c](c, true, *this);
		}
	} else {
		// the partner wants to use the option
		if( m_partner_option_handler[(size_t)c] ) {
			// the option is acceptable
			// the partner can use the option
			// reply DO
			m_partner_option_handler[(size_t)c](c, true, *this);
			owrite3(IAC, DO, c);
		} else {
			// the option is not acceptable
			// reply DONT
			owrite3(IAC, DONT, c);
		}
	}
	m_istate = NULL;
}

void emtelnet::istate_WONT(byte c)
{
	//fprintf(stderr, "wont %u\n", c);
	if( m_will_waiting.test(c) ) {
		// do or dont is sent and receive wont
		// the partner can't use the option
		m_will_waiting.reset(c);
	} else {
		// I am required not to use the option
		// I can't use the option
		// reply wont
		if( m_my_option_handler[(size_t)c] ) {
			m_my_option_handler[(size_t)c](c, false, *this);
		}
		owrite3(IAC, WONT, c);
	}
	m_istate = NULL;
}

void emtelnet::istate_DO(byte c)
{
	//fprintf(stderr, "do %u\n", c);
	if( m_do_waiting.test(c) ) {
		// will (or maybe wont) is sent and receive do
		// I can use the option
		m_do_waiting.reset(c);
		if( m_my_option_handler[(size_t)c] ) {
			m_my_option_handler[(size_t)c](c, true, *this);
		}
	} else {
		// I am required to use use the option
		if( m_my_option_handler[(size_t)c] ) {
			// I support the option
			// I can use the option
			// reply will
			m_my_option_handler[(size_t)c](c, true, *this);
			owrite3(IAC, WILL, c);
		} else {
			// I don't support the option
			// reply wont
			owrite3(IAC, WONT, c);
		}
	}
	m_istate = NULL;
}

void emtelnet::istate_DONT(byte c)
{
	//fprintf(stderr, "dont %u\n", c);
	if( m_do_waiting.test(c) ) {
		// will is sent and receive dont
		// I can't use the option
		if( m_my_option_handler[(size_t)c] ) {
			m_my_option_handler[(size_t)c](c, false, *this);
		}
	} else {
		// the partner wants not to use the option
		// the partner can't use the option
		// reply dont
		if( m_my_option_handler[(size_t)c] ) {
			m_my_option_handler[(size_t)c](c, false, *this);
		}
		owrite3(IAC, WONT, c);
	}
	m_istate = NULL;
}

void emtelnet::istate_SB(byte c)
{
	//fprintf(stderr, "sb %u\n", c);
	m_sb_buffer[0] = c;
	m_sb_length = 1;
	sb_reset(c);
	m_istate = &emtelnet::istate_SB_MSG;
}

void emtelnet::istate_SB_MSG(byte c)
{
	if( c == IAC ) {
		m_istate = &emtelnet::istate_SB_MSG_IAC;
	} else {
		if( !sb_write(c) ) {
			// subnegotiation is too long
			m_istate = NULL;
		}
	}
}

void emtelnet::istate_SB_MSG_IAC(byte c)
{
	if( c == IAC ) {
		// IAC-escaped IAC. write one IAC
		if( !sb_write(IAC) ) {
			// subnegotiation is too long
			m_istate = NULL;
		} else {
			m_istate = &emtelnet::istate_SB_MSG;
		}
	} else if( c == SE ) {
		// end of subnegotiation
		if( m_sb_handler[ (size_t)m_sb_buffer[0] ] ) {
			m_sb_handler[ (size_t)m_sb_buffer[0] ](
					m_sb_buffer[0],
					m_sb_buffer+1,
					m_sb_length-1,
					*this );
		}
		m_istate = NULL;
	}
}


void emtelnet::sb_reset(byte cmd)
{
	m_sb_buffer[0] = cmd;
	m_sb_length = 1;
}

bool emtelnet::sb_write(byte c)
{
	if( SB_MAX_LENGTH <= m_sb_length ) {
		return false;
	}
	m_sb_buffer[m_sb_length++] = c;
	return true;
}


