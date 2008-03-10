/*
 * This file is part of Partty
 *
 * Copyright (C) 2008-2009 FURUHASHI Sadayuki
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

struct usage_action {
	unsigned int operator() (int argc, char* argv[]) {
		usage();
		exit(0);
	}
};

#ifndef DISABLE_IPV6
int listen_socket(struct sockaddr_storage& saddr)
#else
int listen_socket(struct sockaddr_in& saddr)
#endif
{
#ifndef DISABLE_IPV6
	int ssock = socket(saddr.ss_family, SOCK_STREAM, 0);
#else
	int ssock = socket(PF_INET, SOCK_STREAM, 0);
#endif
	if( ssock < 0 ) {
		perror("socket");
		return -1;
	}
	int on = 1;
	if( setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ) {
		perror("setsockopt");
		return -1;
	}
#ifndef DISABLE_IPV6
	socklen_t saddr_len = (saddr.ss_family == AF_INET) ?
		sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6);
#else
	socklen_t saddr_len = sizeof(struct sockaddr_in);
#endif
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

#ifndef DISABLE_IPV6
	struct sockaddr_storage saddr;
#else
	struct sockaddr_in saddr;
#endif
	std::string gate_dir;
	std::string archive_dir;
	bool gate_dir_set;
	bool archive_dir_set;
	try {
		using namespace Kazuhiki;
		--argc;  ++argv;  // skip argv[0]
		Parser kz;
		kz.on("-g", "--gate",    Accept::String(gate_dir), gate_dir_set);
		kz.on("-a", "--archive", Accept::String(archive_dir), archive_dir_set);
		kz.on("-h", "--help",    Accept::Action(usage_action()));
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

