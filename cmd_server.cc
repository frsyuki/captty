#include "partty.h"
#include "uniext.h"
#include "fdtransport.h"
#include <kazuhiki/network.h>
#include <string.h>
#include <string>
#include <limits.h>
#include <signal.h>

void usage(void)
{
	std::cout
		<< "\n"
		<< "* Partty Server (listen host and serve session)\n"
		<< "   [listen server]$ partty-server  [options]  # use default port ["<<Partty::SERVER_DEFAULT_PORT<<"]\n"
		<< "   [listen server]$ partty-server  [options]  <port number>\n"
		<< "   [listen server]$ partty-server  [options]  <listen address>[:<port number>]\n"
		<< "   [listen server]$ partty-server  [options]  <network interface name>[:<port number>]\n"
		<< "\n"
		<< "   options:\n"
		<< "     -g <gate directory>      ["<<PARTTY_GATE_DIR<<"]\n"
		<< "     -a <archive directory>   ["<<PARTTY_ARCHIVE_DIR<<"]\n"
		<< "\n"
		<< std::endl;
}

int listen_socket(struct sockaddr_storage& saddr)
{
	int ssock = socket(saddr.ss_family, SOCK_STREAM, 0);
	if( ssock < 0 ) {
		perror("socket");
		return -1;
	}
	int on = 1;
	if( setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ) {
		perror("setsockopt");
		return -1;
	}
	socklen_t saddr_len = (saddr.ss_family == AF_INET) ?
		sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);
	if( bind(ssock, (struct sockaddr*)&saddr, saddr_len) < 0 ) {
		perror("can't bind the address");
		return -1;
	}
	if( listen(ssock, 5) < 0 ) {
		perror("listen");
		return -1;
	}
	return ssock;
}

int main(int argc, char* argv[], char* envp[])
{
	// Server needs initprocname
	Partty::initprocname(argc, argv, envp);

	struct sockaddr_storage saddr;
	std::string gate_dir;
	std::string archive_dir;
	bool gate_dir_set;
	bool archive_dir_set;
	try {
		using namespace Kazuhiki;
		--argc;  ++argv;  // skip argv[0]
		Parser kz;
		kz.on("-g", "--gate", Accept::String(gate_dir), gate_dir_set);
		kz.on("-a", "--archive", Accept::String(archive_dir), archive_dir_set);
		kz.break_parse(argc, argv);
		if( argc == 0 ) {
			Convert::AnyAddress(saddr, Partty::SERVER_DEFAULT_PORT);
		} else if( argc == 1 ) {
			Convert::FlexibleListtenAddress(argv[0], saddr, Partty::SERVER_DEFAULT_PORT);
		} else {
			usage();
			return 1;
		}

	} catch (Kazuhiki::ArgumentError& e) {
		std::cerr << "error: " << e.what() << std::endl;
		usage();
		return 1;
	}

	int ssock = listen_socket(saddr);
	if( ssock < 0 ) {
		return 1;
	}

	{  // Server needs SIGCHLD handler
		struct sigaction act_child;
		act_child.sa_handler = SIG_IGN;
		sigemptyset(&act_child.sa_mask);
		act_child.sa_flags = SA_NOCLDSTOP | SA_RESTART;
		sigaction(SIGCHLD, &act_child, NULL);
	}

	try {
		Partty::Server::config_t config(ssock);
		if(gate_dir_set) {
			config.gate_dir = gate_dir.c_str();
		}
		if(archive_dir_set) {
			config.archive_dir = archive_dir.c_str();
		}
		Partty::Server server(config);
		return server.run();
	} catch (Partty::partty_error& e) {
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}
}

