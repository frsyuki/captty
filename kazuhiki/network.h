#ifndef KAZUHIKI_NETWORK_H__
#define KAZUHIKI_NETWORK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include "kazuhiki/parser.h"
#include "kazuhiki/basic.h"

namespace Kazuhiki {
namespace Accept {


namespace Network {
template <typename Address>
bool ResolveHostNameIMPL(const char* str, Address& addr, bool resolve, int family, int flags,unsigned short port)
{
	Address v;
	memset(&v, 0, sizeof(v));
	if( !resolve ) {
		if( family == AF_UNSPEC ) {
			if( inet_pton(AF_INET,  str, &((struct sockaddr_in* )&v)->sin_addr) > 0 ){
				((struct sockaddr_in*)&v)->sin_family = AF_INET;
				((struct sockaddr_in*)&v)->sin_len = sizeof(struct sockaddr_in);
			} else if( inet_pton(AF_INET6, str, &((struct sockaddr_in6*)&v)->sin6_addr) > 0 ) {
				((struct sockaddr_in6*)&v)->sin6_family = AF_INET6;
				((struct sockaddr_in6*)&v)->sin6_len = sizeof(struct sockaddr_in6);
			} else {
				return false;
			}
		} else if( family == AF_INET ) {
			if( inet_pton(AF_INET,  str, &((struct sockaddr_in* )&v)->sin_addr) > 0 ){
				((struct sockaddr_in*)&v)->sin_family = AF_INET;
				((struct sockaddr_in*)&v)->sin_len = sizeof(struct sockaddr_in);
			} else {
				return false;
			}
		} else {  // family == AF_INET6
			if( inet_pton(AF_INET6, str, &((struct sockaddr_in6*)&v)->sin6_addr) > 0 ) {
				((struct sockaddr_in6*)&v)->sin6_family = AF_INET6;
				((struct sockaddr_in6*)&v)->sin6_len = sizeof(struct sockaddr_in6);
			} else {
				return false;
			}
		}
	} else {
		struct addrinfo hints;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = family;
		hints.ai_socktype = SOCK_STREAM;    // TODO: 指定する？
		hints.ai_flags = flags;
		struct addrinfo *res = NULL;
		int err;
		if( (err=getaddrinfo(str, NULL, &hints, &res)) != 0 ) {
			return false;
		}
		struct addrinfo* rp(res);  // 最初の一つを使う
		memcpy(&v, rp->ai_addr, rp->ai_addrlen);
		freeaddrinfo(res);
	}
	addr = v;
	((struct sockaddr_in*)&addr)->sin_port = htons(port);
	return true;
}
bool HostName(const char* str, struct sockaddr_in& addr, unsigned short port, bool resolve = true) {
	return ResolveHostNameIMPL(str, addr, resolve, AF_INET, 0, port);
}
bool HostName(const char* str, struct sockaddr_in6& addr, unsigned short port, bool resolve = true) {
	return ResolveHostNameIMPL(str, addr, resolve, AF_INET6, 0, port);
}
bool HostName(const char* str, struct sockaddr_storage& addr, unsigned short port, bool resolve = true) {
	return ResolveHostNameIMPL(str, addr, resolve, AF_UNSPEC, AI_ADDRCONFIG, port);
}

bool AnyAddress(struct sockaddr_in& addr, unsigned short port) {
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	return true;
}
bool AnyAddress(struct sockaddr_in6& addr, unsigned short port) {
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	addr.sin6_addr = in6addr_any;
	return true;
}
bool AnyAddress(struct sockaddr_storage& addr, unsigned short port) {
	memset(&addr, 0, sizeof(addr));
	addr.ss_family = AF_INET6;
	((struct sockaddr_in6*)&addr)->sin6_port = htons(port);
	((struct sockaddr_in6*)&addr)->sin6_addr = in6addr_any;
	return true;
}

template <typename Address>
bool NetworkInterfaceIMPL(const char* str, Address& addr, int family, unsigned short port) {
	struct ifaddrs* ifap;
	if( getifaddrs(&ifap) ) {
		return false;
	}
	for(struct ifaddrs* i(ifap); i != NULL; i = i->ifa_next) {
		if( i->ifa_addr == NULL) { continue; }
		if( str != i->ifa_name ) { continue; }
		if( family == AF_UNSPEC || i->ifa_addr->sa_family == family ) {
			memcpy(&addr, i->ifa_addr, sizeof(addr));
			((struct sockaddr_in*)&addr)->sin_port = htons(port);
			freeifaddrs(ifap);
			return true;
		}
	}
	freeifaddrs(ifap);
	return false;
}
bool NetworkInterface(const char* str, struct sockaddr_in& addr, unsigned short port) {
	return NetworkInterfaceIMPL(str, addr, AF_INET, port);
}
bool NetworkInterface(const char* str, struct sockaddr_in6& addr, unsigned short port) {
	return NetworkInterfaceIMPL(str, addr, AF_INET6, port);
}
bool NetworkInterface(const char* str, struct sockaddr_storage& addr, unsigned short port) {
	return NetworkInterfaceIMPL(str, addr, AF_UNSPEC, port);
}

}  // namespace Network


template <typename Address>
struct HostNameIMPL : Acceptable {
	HostNameIMPL(Address& d) : data(d) {}
	unsigned int operator() (int argc, char* argv[]) {
		if( argc < 1 ) { throw LackOfArgumentsError("Host name is required"); }
		if( !Network::HostName(argv[0], data, 0, true) ) {
			throw InvalidArgumentError("Invalid host name");
		}
		return 1;
	}
private:
	Address& data;
};
template <typename Address>
HostNameIMPL<Address> HostName(Address& a) { return HostNameIMPL<Address>(a); }


template <typename Address>
struct IPAddressIMPL : Acceptable {
	IPAddressIMPL(Address& d) : data(d) {}
	unsigned int operator() (int argc, char* argv[]) {
		if( argc < 1 ) { throw LackOfArgumentsError("IP address is required"); }
		if( !Network::HostName(argv[0], data, 0, false) ) {
			throw InvalidArgumentError("Invalid address");
		}
		return 1;
	}
private:
	Address& data;
};
template <typename Address>
IPAddressIMPL<Address> IPAddress(Address& a) { return IPAddressIMPL<Address>(a); }



////
// Flexible Passive Address
//
// 1. interface:port
// 2. ip.add.re.ss:port
// 3. [ip:add::re:ss]:port   (ipv6)
// 4. :port
// 5. port
// 6. interface      (default port)
// 7. ip.add.re.ss   (default port)
// 8. ip:add::re:ss  (default port, ipv6)
//
template <typename Address>
struct FlexibleListtenAddressIMPL : Acceptable {
	FlexibleListtenAddressIMPL(Address& addr, unsigned short default_port, bool resolve) :
			raddr(addr), dport(default_port), do_resolve(resolve) {}
	unsigned int operator() (int argc, char* argv[]) {
		if( argc < 1 ) { throw LackOfArgumentsError("Network interface name or address:port is required"); }
		if( !parse(argv[0]) ) {
			throw InvalidArgumentError("Network interface name or address:port is required");
		}
		return 1;
	}
private:
	bool parse(const std::string& str) {	
		std::string::size_type posc;
		if( (posc = str.rfind(':')) != std::string::npos ) {
			// 1 .. 4, 8
			if( str.find(':') != posc ) {
				// 3, 8
				if( posc != 0 && str[posc-1] == ']' && str[0] == '[' ) {
					// 3
					std::string host( str.substr(1,posc-2) );
					std::string port( str.substr(posc+1) );
					return resolve_addr(host.c_str(), port.c_str());
				} else {
					// 8
					return resolve_addr(str.c_str(), NULL);
				}
			} else {
				// 1, 2, 4
				std::string host( str.substr(0,posc) );
				std::string port( str.substr(posc+1) );
				if( host.empty() ) {
					// 4
					return resolve_any(port.c_str());
				} else if( resolve_addr(host.c_str(), port.c_str()) ) {
					// 2
					return true;
				} else {
					// 1
					return resolve_netif(host.c_str(), port.c_str());
				}
			}
		} else {
			// 5 .. 7
			uint16_t p;
			if( Basic::Numeric(str.c_str(), p) ) {
				// 5
				return resolve_any(str.c_str());
			} else if( resolve_addr(str.c_str(), NULL) ) {
				// 7
				return true;
			} else {
				// 6
				return resolve_addr(str.c_str(), NULL);
			}
		}
	}
	bool resolve_addr(const char* addr, const char* port)
	{
		unsigned short p = dport;
		if( port && !Basic::Numeric(port, p) ) {
			return false;
		}
		return Network::HostName(addr, raddr, p, do_resolve);
	}
	bool resolve_netif(const char* netif, const char* port)
	{
		unsigned short p = dport;
		if( port && !Basic::Numeric(port, p) ) {
			return false;
		}
		return Network::NetworkInterface(netif, raddr, p);
	}
	bool resolve_any(const char* port)
	{
		// FIXME
		unsigned short p = dport;
		if( port && !Basic::Numeric(port, p) ) {
			return false;
		}
		return Network::AnyAddress(raddr, p);
	}
private:
	Address& raddr;
	unsigned short dport;
	bool do_resolve;
};
template <typename Address>
FlexibleListtenAddressIMPL<Address> FlexibleListtenAddress(Address& a, unsigned short default_port, bool resolve = true) {
	return FlexibleListtenAddressIMPL<Address>(a, default_port, resolve);
}



////
// Flexible Active Host
//
// 1. host:port
// 2. ip.add.re.ss:port
// 3. [ip:add::re:ss]:port   (ipv6)
// 4. host           (default port)
// 5. ip.add.re.ss   (default port)
// 6. ip:add::re:ss  (default port, ipv6)
//
template <typename Address>
struct FlexibleActiveHostIMPL : Acceptable {
	FlexibleActiveHostIMPL(Address& addr, unsigned short default_port, bool resolve) :
			raddr(addr), dport(default_port), do_resolve(resolve) {}
	unsigned int operator() (int argc, char* argv[]) {
		if( argc < 1 ) { throw LackOfArgumentsError("Host name is required"); }
		if( !parse(argv[0]) ) {
			throw InvalidArgumentError("Host name is required");
		}
		return 1;
	}
private:
	bool parse(const std::string& str) {
		std::string::size_type posc;
		if( (posc = str.rfind(':')) != std::string::npos ) {
			// 1 .. 3, 6
			if( str.find(':') != posc ) {
				// 3, 6
				if( posc != 0 && str[posc-1] == ']' && str[0] == '[' ) {
					// 3
					std::string host( str.substr(1,posc-2) );
					std::string port( str.substr(posc+1) );
					return resolve_addr(host.c_str(), port.c_str());
				} else {
					// 6
					return resolve_addr(str.c_str(), NULL);
				}
			} else {
				// 1, 2
				// 1と2は区別しない
				std::string host( str.substr(0,posc) );
				std::string port( str.substr(posc+1) );
				return resolve_addr(host.c_str(), port.c_str());
			}
		} else {
			// 4, 5
			// 4と5は区別しない
			return resolve_addr(str.c_str(), NULL);
		}
	}
	bool resolve_addr(const char* addr, const char* port) {
		unsigned short p = dport;
		if( port && !Basic::Numeric(port, p) ) {
			return false;
		}
		return Network::HostName(addr, raddr, p, do_resolve);
	}
private:
	Address& raddr;
	unsigned short dport;
	bool do_resolve;
};

template <typename Address>
FlexibleActiveHostIMPL<Address> FlexibleActiveHost(Address& a, uint16_t default_port, bool resolve = true) {
	return FlexibleActiveHostIMPL<Address>(a, default_port, resolve);
}


}  // namespace Accept
}  // namespace Kazuhiki

#endif /* kazuhiki/network.h */

