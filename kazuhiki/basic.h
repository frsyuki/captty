#ifndef KAZUHIKI_BASIC_H__
#define KAZUHIKI_BASIC_H__

#include <cstring>
#include "kazuhiki/parser.h"
#include "kazuhiki/basic_impl.h"

namespace Kazuhiki {


namespace Accept {
	struct Boolean : Acceptable {
		Boolean(bool& d) : data(d) {}
		unsigned int operator() (int argc, char* argv[]) {
			if( argc > 0 && IMPL::ConvertBoolean(argv[0], data) ) {
				return 1;
			} else {
				data = true;
				return 0;
			}
		}
	private:
		bool& data;
		Boolean();
	};
	
	
	struct String : Acceptable {
		String(std::string& d) : data(d) {}
		unsigned int operator() (int argc, char* argv[]) {
			if( argc < 1 ) { throw LackOfArgumentsError("More argument rquired"); }
			data = argv[0];
			return 1;
		}
	private:
		std::string& data;
		String();
	};
	
	
	template <typename T>
	struct IMPL::NumericIMPL<T> Numeric(T& n) { return IMPL::NumericIMPL<T>(n); }
}


namespace Convert {

	void Boolean(char* arg, bool& d) {
		Accept::Boolean f(d);
		f(1, &arg);
	}

	void String(char* arg, std::string& d) {
		d = arg;
	}

	template <typename T>
	void Numeric(char* arg, T& n) {
		IMPL::NumericIMPL<T> f(n);
		f(1, &arg);
	}
}


}  // namespace Kazuhiki

#endif /* kazuhiki/basic.h */

