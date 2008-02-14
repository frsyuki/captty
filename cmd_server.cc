#include "partty.h"
#include "unio.h"
#include "fdtransport.h"
#include "kazuhiki/network.h"
#include <string.h>
#include <string>
#include <limits.h>

int main(int argc, char* argv[])
{
	// FIXME GATE_DIR
	//const char* session = "test";

	struct sockaddr_storage saddr;
	std::string session;
	{
		using namespace Kazuhiki;
		Parser kz;
		kz.on("-l", "", Accept::FlexibleListtenAddress(saddr, 2755));
		kz.on("-s", "", Accept::String(session));
		--argc;
		++argv;
		kz.break_order(argc, argv);
	}

	int ssock = socket(saddr.ss_family, SOCK_STREAM, 0);
	if( ssock < 0 ) { pexit("ssock"); }
	int on = 1;
	if( setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ) {
		pexit("setsockopt"); }
	if( bind(ssock, (struct sockaddr*)&saddr, saddr.ss_len) < 0 ) {
		pexit("bind"); }
	if( listen(ssock, 5) < 0 ) { pexit("listen"); }

	char gate_path[PATH_MAX + Partty::MAX_SESSION_NAME_LENGTH];
	ssize_t gate_dir_len = strlen(Partty::GATE_DIR);
	memcpy(gate_path, Partty::GATE_DIR, gate_dir_len);
	strlcpy(gate_path + gate_dir_len, session.c_str(), sizeof(gate_path));
	int gate = listen_gate(gate_path);
	if( gate < 0 ) { pexit("listen_gate"); }

	int host = accept(ssock, NULL, NULL);
	if( host < 0 ) { pexit("accept host"); }
	close(ssock);  // FIXME

	Partty::Server server(
			host, gate,
			session.c_str(), session.length(),
			"", 0);
	return server.run();

	// FIXME unlink
}

