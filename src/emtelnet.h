/*
 * emtelnet
 *
 * Copyright (C) 2008-2009 FURUHASHI Sadayuki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef EMTELNET_H__
#define EMTELNET_H__ 1

#include <cstddef>
#include <stdexcept>
#include <bitset>

class emtelnet {
public:
	emtelnet(void* user);
	virtual ~emtelnet();
	typedef unsigned char byte;
public:
	// process `buf' to send it to partner
	// and save the result to obffer
	void send(const void* buf, size_t len);

	// process `buf' as received data from partner
	// and save the result to ibuffer
	void recv(const void* buf, size_t len);

	void send_will(byte cmd);
	void send_wont(byte cmd);
	void send_do(byte cmd);
	void send_dont(byte cmd);
	void send_sb(byte cmd, const void* msg, size_t len);
public:
	typedef void (*my_option_handler_t)(char cmd, bool sw, emtelnet& self);
	typedef void (*partner_option_handler_t)(char cmd, bool sw, emtelnet& self);
	typedef void (*sb_handler_t)(char cmd, const char* msg, size_t len, emtelnet& self);
	inline void set_partner_option_handler(char cmd, partner_option_handler_t handler) {
		m_partner_option_handler[(size_t)cmd] = handler; }
	inline void set_my_option_handler(char cmd, my_option_handler_t handler) {
		m_my_option_handler[(size_t)cmd] = handler; }
	inline void set_sb_handler(char cmd, sb_handler_t handler) {
		m_sb_handler[(size_t)cmd] = handler; }
public:
	size_t oalloced;
	size_t olength;
	char* obuffer;
	void clear_obuffer(void) { olength = 0; }
	size_t ialloced;
	size_t ilength;
	char* ibuffer;
	void clear_ibuffer(void) { ilength = 0; }
	void* user;
private:
	emtelnet();
	emtelnet(const emtelnet&);
private:
	static const size_t SB_MAX_LENGTH = 1024;  // FIXME
	size_t m_sb_length;
	char m_sb_buffer[SB_MAX_LENGTH];
	void sb_reset(byte cmd);
	bool sb_write(byte c);
private:
	my_option_handler_t m_my_option_handler[256];
	partner_option_handler_t m_partner_option_handler[256];
	sb_handler_t m_sb_handler[256];
	std::bitset<256> m_will_waiting;  // bitset is smaller than standard array of bool
	std::bitset<256> m_do_waiting;
private:
	bool m_ostate_cr;
	void (emtelnet::*m_istate)(byte c);
private:
	inline void istate_NORMAL(byte c);
	void istate_CR(byte c);
	void istate_IAC(byte c);
	void istate_WILL(byte c);
	void istate_WONT(byte c);
	void istate_DO(byte c);
	void istate_DONT(byte c);
	void istate_SB(byte c);
	void istate_SB_MSG(byte c);
	void istate_SB_MSG_IAC(byte c);
private:
	inline void owrite1(byte c);
	inline void owrite2(byte c1, byte c2);
	inline void owrite3(byte c1, byte c2, byte c3);
	inline void owrite4(byte c1, byte c2, byte c3, byte c4);
	inline void owrite(const byte* s, size_t len);
	inline void orealloc(size_t reqlen);
	inline void iwrite(byte c);
	inline void iwrite(const byte* s, size_t len);
	inline void irealloc(size_t reqlen);
public:
	// from putty-0.60/telnet.c
	// http://www.iana.org/assignments/telnet-options
	static const byte LF	= 0x0A;  // ASCII LF
	static const byte CR	= 0x0D;  // ASCII CR
	static const byte IAC	= 255;   // interrupt as command
	static const byte WILL	= 251;   // I will use option
	static const byte WONT	= 252;   // I won't use option
	static const byte DO	= 253;   // please, you use option
	static const byte DONT	= 254;   // you are not to use option
	static const byte SB	= 250;   // interrupt as subnegotiation
	static const byte SE	= 240;   // end subnegotiation

	static const byte GA		= 249;  // you may reverse the line
	static const byte EL		= 248;  // erase the current line
	static const byte EC		= 247;  // erase the current byteacter
	static const byte AYT		= 246;  // are you there
	static const byte AO		= 245;  // abort output--but let prog finish
	static const byte IP		= 244;  // interrupt process--permanently
	static const byte BREAK		= 243;  // break
	static const byte DM		= 242;  // data mark--for connect. cleaning
	static const byte NOP		= 241;  // nop
	static const byte EOR		= 239;  // end of record (transparent mode)
	static const byte ABORT		= 238;  // Abort process
	static const byte SUSP		= 237;  // Suspend process
	static const byte xEOF		= 236;  // End of file: EOF is already used...

	static const byte OPT_BINARY	=   0;  // 8-bit data path
	static const byte OPT_ECHO	=   1;  // echo
	static const byte OPT_RCP	=   2;  // prepare to reconnect
	static const byte OPT_SGA	=   3;  // suppress go ahead
	static const byte OPT_NAMS	=   4;  // approximate message size
	static const byte OPT_STATUS	=   5;  // give status
	static const byte OPT_TM	=   6;  // timing mark
	static const byte OPT_RCTE	=   7;  // remote controlled transmission and echo
	static const byte OPT_NAOL	=   8;  // negotiate about output line width
	static const byte OPT_NAOP 	=   9;  // negotiate about output page size
	static const byte OPT_NAOCRD	=  10;  // negotiate about CR disposition
	static const byte OPT_NAOHTS	=  11;  // negotiate about horizontal tabstops
	static const byte OPT_NAOHTD	=  12;  // negotiate about horizontal tab disposition
	static const byte OPT_NAOFFD	=  13;  // negotiate about formfeed disposition
	static const byte OPT_NAOVTS	=  14;  // negotiate about vertical tab stops
	static const byte OPT_NAOVTD	=  15;  // negotiate about vertical tab disposition
	static const byte OPT_NAOLFD	=  16;  // negotiate about output LF disposition
	static const byte OPT_XASCII	=  17;  // extended ascic byteacter set
	static const byte OPT_LOGOUT	=  18;  // force logout
	static const byte OPT_BM	=  19;  // byte macro
	static const byte OPT_DET	=  20;  // data entry terminal
	static const byte OPT_SUPDUP	=  21;  // supdup protocol
	static const byte OPT_SUPDUPOUTPUT=22;  // supdup output
	static const byte OPT_SNDLOC	=  23;  // send location
	static const byte OPT_TTYPE	=  24;  // terminal type
	static const byte OPT_EOR	=  25;  // end or record
	static const byte OPT_TUID	=  26;  // TACACS user identification
	static const byte OPT_OUTMRK	=  27;  // output marking
	static const byte OPT_TTYLOC	=  28;  // terminal location number
	static const byte OPT_3270REGIME=  29;  // 3270 regime
	static const byte OPT_X3PAD	=  30;  // X.3 PAD
	static const byte OPT_NAWS	=  31;  // window size
	static const byte OPT_TSPEED	=  32;  // terminal speed
	static const byte OPT_LFLOW	=  33;  // remote flow control
	static const byte OPT_LINEMODE	=  34;  // Linemode option
	static const byte OPT_XDISPLOC	=  35;  // X Display Location
	static const byte OPT_OLD_ENVIRON= 36;  // Old - Environment variables
	static const byte OPT_AUTHENTICATION = 37;  //  Authenticate
	static const byte OPT_ENCRYPT	=  38;  // Encryption option
	static const byte OPT_NEW_ENVIRON= 39;  // New - Environment variables
	static const byte OPT_TN3270E	=  40;  // TN3270 enhancements
	static const byte OPT_XAUTH	=  41;  // 
	static const byte OPT_byteSET	=  42;  // byteacter set
	static const byte OPT_RSP	=  43;  // Remote serial port
	static const byte OPT_COM_PORT_OPTION = 44;  //  Com port control
	static const byte OPT_SLE	=  45;  // Suppress local echo
	static const byte OPT_STARTTLS	=  46;  // Start TLS
	static const byte OPT_KERMIT	=  47;  // Automatic Kermit file transfer
	static const byte OPT_SEND_URL	=  48;
	static const byte OPT_FORWARD_X	=  49;
	static const byte OPT_PRAGMA_LOGON = 138;
	static const byte OPT_SSPI_LOGON= 139;
	static const byte OPT_EXOPL	= 255;  // extended-options-list
	static const byte OPT_PRAGMA_HEARTBEAT = 140;
};


#endif /* emtelnet.h */

