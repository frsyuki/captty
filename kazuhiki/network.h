#ifndef KAZUHIKI_NETWORK_H__
#define KAZUHIKI_NETWORK_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifndef DISABLE_IFADDRS
#include <ifaddrs.h>
#endif
#include "kazuhiki/parser.h"
#include "kazuhiki/basic.h"
#include "kazuhiki/network_impl.h"

namespace Kazuhiki {


namespace Accept {

	template <typename Address>
	IMPL::HostNameIMPL<Address> HostName(Address& a) {
		return IMPL::HostNameIMPL<Address>(a);
	}

	template <typename Address>
	IMPL::IPAddressIMPL<Address> IPAddress(Address& a) {
		return IMPL::IPAddressIMPL<Address>(a);
	}

	template <typename Address>
	IMPL::FlexibleListtenAddressIMPL<Address>
	FlexibleListtenAddress(Address& a, unsigned short default_port,
				bool resolve = true) {
		return IMPL::FlexibleListtenAddressIMPL<Address>(a, default_port, resolve);
	}

	template <typename Address>
	IMPL::FlexibleActiveHostIMPL<Address>
	FlexibleActiveHost(Address& a, uint16_t default_port, bool resolve = true) {
		return IMPL::FlexibleActiveHostIMPL<Address>(a, default_port, resolve);
	}

}

namespace Convert {

	template <typename Address>
	void HostName(char* arg, Address& a) {
		IMPL::HostNameIMPL<Address> f(a);
		f(1, &arg);
	}

	template <typename Address>
	void IPAddress(char* arg, Address& a) {
		IMPL::IPAddressIMPL<Address> f(a);
		f(1, &arg);
	}

	template <typename Address>
	void FlexibleListtenAddress(char* arg,
				Address& a, unsigned short default_port,
				bool resolve = true) {
		IMPL::FlexibleListtenAddressIMPL<Address> f(a, default_port, resolve);
		f(1, &arg);
	}

	template <typename Address>
	void FlexibleActiveHost(char* arg,
				Address& a, uint16_t default_port, bool resolve = true) {
		IMPL::FlexibleActiveHostIMPL<Address> f(a, default_port, resolve);
		f(1, &arg);
	}

	template <typename Address>
	void AnyAddress(Address& a, uint16_t port) {
		IMPL::ConvertAnyAddress(a, port);
	}
}


}  // namespace Kazuhiki

#endif /* kazuhiki/network.h */

