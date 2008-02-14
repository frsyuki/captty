#include "partty.h"
#include "unio.h"

int main(void)
{
	int sock = listen_tcp(4000);
	if( sock < 0 ) { pexit("listen_tcp"); }
	Partty::Gate gate(sock);
	return gate.run();
}

