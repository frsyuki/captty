#include "partty.h"
#include "unio.h"
#include "kazuhiki/basic.h"
#include "kazuhiki/network.h"
#include <iostream>

struct option {
	bool verbose;
};


void usage(void)
{
	std::cout
		<< "\n"
		<< "* Partty Host (create new session)\n"
		<< "   [listen on TCP  ]$ partty  host  # use default port []\n"
		<< "   [listen on TCP  ]$ partty  host  <port number>\n"
		<< "   [listen on TCP  ]$ partty  host  <network interface name>[:<port number>]\n"
		<< "   [listen on TCP  ]$ partty  host  <listen address>[:<port number>]\n"
		<< "   [lisetn on Local]$ partty  host  -l  <path to socket to create>\n"
		<< "\n"
		<< "* Partty Guest (join existent session)\n"
		<< "   [connect to TCP  ]$ partty  guest  <hostname>  # use default port []\n"
		<< "   [connect to TCP  ]$ partty  guest  <hostname>:<port number>\n"
		<< "   [connect to Local]$ partty  guest  -l  <path to socket>\n"
		<< "\n"
		<< "* Partty Exit Command\n"
		<< "   [show sessions   ]$ partty  exit\n"
		<< "   [exit session    ]$ partty  exit <session number>\n"
		<< std::endl;
}

int main(int argc, char* argv[])
{
	struct option opt;
	struct sockaddr_storage saddr;
	try {
		using namespace Kazuhiki;
		--argc;  // skip argv[0]
		++argv;
		Parser kz;
		kz.on("-v", "--verbose", Accept::Boolean(opt.verbose));
		kz.break_parse(argc, argv);
		if( argc < 1 ) { throw ArgumentError(""); }
		Convert::FlexibleActiveHost(argv[0], saddr, 2755);

	} catch (Kazuhiki::ArgumentError& e) {
		std::cout << e.what() << std::endl;
		usage();
		return 1;
	}

	int ssock = socket(saddr.ss_family, SOCK_STREAM, 0);
	if( ssock < 0 ) { pexit("socket"); }
	if( connect(ssock, (struct sockaddr*)&saddr, saddr.ss_len) < 0 ) {
		pexit("bind");
	}

	Partty::Host host(ssock, "test", 4, "", 0);
	return host.run();
}

