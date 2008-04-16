/*
 * This file is part of Partty
 *
 * Copyright (C) 2007-2008 FURUHASHI Sadayuki
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

#include "partty.h"
#include "uniext.h"
#include <kazuhiki/basic.h>
#include <kazuhiki/network.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>

namespace {

char* getusername(void) {
	static char unknown_user[] = "nobody";
	struct passwd* pw = getpwuid(geteuid());
	if(pw == NULL) {
		return unknown_user;
	} else {
		return pw->pw_name;
	}
}

static char* fallback_shell[3];

void usage(void)
{
	std::cout
		<< "\n"
		<< "* Partty Host (create new session)\n"
		<< "   [connect to server]$ partty-host  [options]  <server name>  # use default port ["<<Partty::SERVER_DEFAULT_PORT<<"]\n"
		<< "   [connect to server]$ partty-host  [options]  <server name>:<port number>\n"
		<< "   [example 1        ]$ partty-host  partty.net\n"
		<< "   [example 2        ]$ partty-host  -s mysession  -c 'a'  partty.net\n"
		<< "\n"
		<< "   options:\n"
		<< "     -s <session name>        session name\n"
		<< "     -m <message>             message in a word ["<<getusername()<<"]\n"
		<< "     -w <oparation password>  password to operate the session\n"
		<< "     -r <view-only password>  password to view the session\n"
		<< "     -k                       disable all gust operation regardless of operation password\n"
		<< "     -c <lock character>      control key to lock guest operation (default: ']')\n"
		<< "\n"
		<< std::endl;
}

struct usage_action {
	unsigned int operator() (int argc, char* argv[]) {
		usage();
		exit(0);
	}
};

struct version_action {
	unsigned int operator() (int argc, char* argv[]) {
		std::cout << "partty-host " << VERSION << std::endl
			  << "\n"
			  << "Copyright (C) 2007-2008 FURUHASHI Sadayuki\n"
			  << "This is free software.  You may redistribute copies of it under the terms of\n"
			  << "the GNU General Public License <http://www.gnu.org/licenses/gpl.html>.\n"
			  << "There is NO WARRANTY, to the extent permitted by law.\n"
			  << "\n"
			  << std::endl;
		exit(0);
	}
};

struct parse_end_exception {};
struct parse_end {
	unsigned int operator() (int argc, char* argv[]) {
		throw parse_end_exception();
	}
};

}  // noname namespace

int main(int argc, char* argv[])
{
	{
		fallback_shell[0] = getenv("SHELL");
		if(!fallback_shell[0]) {
#ifdef FALLBACK_SHELL
			fallback_shell[0] = FALLBACK_SHELL;
			fallback_shell[1] = NULL;
#else
			fallback_shell[0] = "/bin/sh";
			fallback_shell[1] = "-i";
			fallback_shell[2] = NULL;
#endif
		}
	}

	std::string session_name;
	std::string message;
	std::string writable_password;
	std::string readonly_password;
	bool view_only = false;
	char lock_char = ']';
	bool session_name_set;
	bool message_set;
	bool writable_password_set;
	bool readonly_password_set;
	bool lock_char_set;
#ifndef DISABLE_IPV6
	struct sockaddr_storage saddr;
#else
	struct sockaddr_in saddr;
#endif
	try {
		using namespace Kazuhiki;
		Parser kz;
		--argc;  ++argv;  // skip argv[0]
		kz.on("-s", "--session",   Accept::String(session_name), session_name_set);
		kz.on("-m", "--message",   Accept::String(message), message_set);
		kz.on("-w", "--password",  Accept::String(writable_password), writable_password_set);
		kz.on("-r", "--view-only", Accept::String(readonly_password), readonly_password_set);
		kz.on("-k", "--locked",    Accept::Boolean(view_only));
		kz.on("-c", "--lock",      Accept::Character(lock_char), lock_char_set);
		kz.on("-h", "--help",      Accept::Action(usage_action()));
		kz.on("-V", "--version",   Accept::Action(version_action()));
		kz.on("--", "",            Accept::Action(parse_end()));
		try {
			kz.break_parse(argc, argv);  // parse!
		} catch (parse_end_exception& e) { }

		if( argc < 1 ) {
			usage();
			return 1;
		}
		Convert::FlexibleActiveHost(argv[0], saddr, Partty::SERVER_DEFAULT_PORT);
		--argc, ++argv;

		if( lock_char < 'a' ) {
			lock_char += 32;  // shift
		}
		if( lock_char < 'a' ) {
			throw ArgumentError("Invalid lock character");
		}

	} catch (Kazuhiki::ArgumentError& e) {
		std::cerr << "error: " << e.what() << std::endl;
		usage();
		return 1;
	}

	// セッション名チェック
	if( !session_name_set ) {
		char sbuf[Partty::MAX_SESSION_NAME_LENGTH+2];  // 長さ超過を検知するために+2
		std::cerr << "Session name: " << std::flush;
		std::cin.getline(sbuf, sizeof(sbuf));
		session_name = sbuf;
	}
	if( session_name.length() > Partty::MAX_SESSION_NAME_LENGTH ) {
		std::cerr << "Session name is too long" << std::endl;
		return 1;
	}
	if( session_name.length() < Partty::MIN_SESSION_NAME_LENGTH ) {
		std::cerr << "Session name is too short" << std::endl;
		return 1;
	}

	// パスワードチェック
	if( !writable_password_set ) {
		char* pass = getpass("Operation password: ");
		writable_password = pass;
	}
	if( writable_password.length() > Partty::MAX_PASSWORD_LENGTH ) {
		std::cerr << "Operation password is too long" << std::endl;
		return 1;
	}

	if( !readonly_password_set ) {
		char* pass = getpass("View-only password: ");
		readonly_password = pass;
	}
	if( readonly_password.length() > Partty::MAX_PASSWORD_LENGTH ) {
		std::cerr << "View-only password is too long" << std::endl;
		return 1;
	}


	// メッセージをチェック
	if( !message_set ) {
		message = getusername();
	}
	if( message.length() > Partty::MAX_MESSAGE_LENGTH ) {
		std::cerr << "Message is too long" << std::endl;
		return 1;
	}

	// ソケットをlisten
#ifndef DISABLE_IPV6
	int ssock = socket(saddr.ss_family, SOCK_STREAM, 0);
#else
	int ssock = socket(PF_INET, SOCK_STREAM, 0);
#endif
	if( ssock < 0 ) {
		perror("socket");
		return 1;
	}
#ifndef DISABLE_IPV6
	socklen_t saddr_len = (saddr.ss_family == AF_INET) ?
		sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);
#else
	socklen_t saddr_len = sizeof(struct sockaddr_in);
#endif
	if( connect(ssock, (struct sockaddr*)&saddr, saddr_len) < 0 ) {
		perror("can't to connect to server");
		return 1;
	}

	Partty::session_info_ref_t info;
	info.message                  = message.c_str();
	info.message_length           = message.length();
	info.session_name             = session_name.c_str();
	info.session_name_length      = session_name.length();
	info.writable_password        = writable_password.c_str();
	info.writable_password_length = writable_password.length();
	info.readonly_password        = readonly_password.c_str();
	info.readonly_password_length = readonly_password.length();

	// 環境変数を設定
	setenv(PARTTY_HOST_ENV_NAME, session_name.c_str(), 1);

	// Host開始
	try {
		Partty::Host::config_t config(ssock, info);
		config.lock_code = lock_char - 'a' + 1;;
		config.view_only = view_only;
		Partty::Host host(config);
		if( argc > 0 ) {
			return host.run(argv);
		} else {
			return host.run(fallback_shell);
		}
	} catch (Partty::partty_error& e) {
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}
}

