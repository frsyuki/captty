#include "partty.h"
#include "unio.h"
#include "fdtransport.h"
#include "kazuhiki/network.h"
#include <string.h>
#include <string>
#include <limits.h>
#include <signal.h>

void usage(void)
{
	std::cout
		<< "\n"
		<< "* Partty Server (listen host and serve session)\n"
		<< "   [listen server]$ partty-server  # use default port ["<<Partty::SERVER_DEFAULT_PORT<<"]\n"
		<< "   [listen server]$ partty-server  <port number>\n"
		<< "   [listen server]$ partty-server  <listen address>[:<port number>]\n"
		<< "   [listen server]$ partty-server  <network interface name>[:<port number>]\n"
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

int main(int argc, char* argv[])
{
	struct sockaddr_storage saddr;
	try {
		using namespace Kazuhiki;
		--argc;  ++argv;  // skip argv[0]
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
		Partty::Server server(ssock);
		return server.run();
	} catch (Partty::partty_error& e) {
		std::cerr << "error: " << e.what() << std::endl;
		return 1;
	}
}

