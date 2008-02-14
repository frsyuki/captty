#include "partty.h"
#include "unio.h"
#include "kazuhiki/basic.h"
#include "kazuhiki/network.h"

struct option {
	bool verbose;
};

int main(int argc, char* argv[])
{
	struct option opt;
	struct sockaddr_storage saddr;
	{
		using namespace Kazuhiki;
		Parser kz;
		kz.on("-s", "", Accept::FlexibleActiveHost(saddr, 2755));
		kz.on("-v", "--verbose", Accept::Boolean(opt.verbose));
		--argc;
		++argv;
		kz.break_order(argc, argv);
	}

	int ssock = socket(saddr.ss_family, SOCK_STREAM, 0);
	if( ssock < 0 ) { pexit("socket"); }
	if( connect(ssock, (struct sockaddr*)&saddr, saddr.ss_len) < 0 ) {
		pexit("bind");
	}

	Partty::Host host(ssock);
	return host.run();
}

