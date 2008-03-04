#include "partty.h"
#include "uniext.h"
#include <kazuhiki/basic.h>
#include <kazuhiki/network.h>
#include <iostream>
#include <string>
#include <stdio.h>
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
		<< "     -c <lock character>      control key to lock guest operation (default: ']')\n"
		<< "\n"
		<< std::endl;
}

}

int main(int argc, char* argv[])
{
	std::string session_name;
	std::string message;
	std::string writable_password;
	std::string readonly_password;
	char lock_char = ']';
	bool session_name_set;
	bool message_set;
	bool writable_password_set;
	bool readonly_password_set;
	bool lock_char_set;
	struct sockaddr_storage saddr;
	try {
		using namespace Kazuhiki;
		Parser kz;
		--argc;  ++argv;  // skip argv[0]
		kz.on("-s", "--session",   Accept::String(session_name), session_name_set);
		kz.on("-m", "--message",   Accept::String(message), message_set);
		kz.on("-w", "--password",  Accept::String(writable_password), writable_password_set);
		kz.on("-r", "--view-only", Accept::String(readonly_password), readonly_password_set);
		kz.on("-c", "--lock",      Accept::Character(lock_char), lock_char_set);
		kz.break_parse(argc, argv);  // parse!

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
	int ssock = socket(saddr.ss_family, SOCK_STREAM, 0);
	if( ssock < 0 ) {
		perror("socket");
		return 1;
	}
	socklen_t saddr_len = (saddr.ss_family == AF_INET) ?
		sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);
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
		Partty::Host host(config);
		if( argc > 0 ) {
			return host.run(argv);
		} else {
			return host.run(NULL);
		}
	} catch (Partty::partty_error& e) {
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}
}

