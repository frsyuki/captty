#include "partty.h"
#include "unio.h"
#include <kazuhiki/basic.h>
#include <kazuhiki/network.h>
#include <iostream>
#include <stdio.h>

#include <pwd.h>
#include <unistd.h>

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
		<< "     -s <session name>      session name\n"
		<< "     -p <password>          password to join the session\n"
		<< "     -c <lock character>    control key to lock guest operation []]\n"
		<< "\n"
		<< std::endl;
}

int main(int argc, char* argv[])
{
	std::string session_name;
	std::string password;
	char lock_char = ']';
	bool session_name_set;
	bool password_set;
	bool lock_char_set;
	struct sockaddr_storage saddr;
	try {
		using namespace Kazuhiki;
		Parser kz;
		--argc;  ++argv;  // skip argv[0]
		kz.on("-s", "--session",  Accept::String(session_name), session_name_set);
		kz.on("-p", "--password", Accept::String(password),     password_set);
		kz.on("-c", "--lock",     Accept::Character(lock_char), lock_char_set);
		kz.break_parse(argc, argv);  // parse!

		if( argc != 1 ) {
			usage();
			return 1;
		}
		Convert::FlexibleActiveHost(argv[0], saddr, Partty::SERVER_DEFAULT_PORT);

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
		char sbuf[Partty::MAX_USER_NAME_LENGTH+2];  // 長さ超過を検知するために+2
		std::cerr << "session name: " << std::flush;
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
	if( !password_set ) {
		char* pass = getpass("password: ");
		password = pass;
	}
	if( password.length() > Partty::MAX_PASSWORD_LENGTH ) {
		std::cerr << "Password is too long" << std::endl;
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

	// Host開始
	try {
		char lock_code = lock_char - 'a' + 1;
		Partty::Host host(ssock, lock_code,
				session_name.c_str(), session_name.length(),
				password.c_str(), password.length());
		return host.run();
	} catch (Partty::partty_error& e) {
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}
}

