#ifndef KAZUHIKI_BASIC_H__
#define KAZUHIKI_BASIC_H__

#include <cstring>
#include "kazuhiki/parser.h"

namespace Kazuhiki {
namespace Accept {


namespace Basic {
bool Boolean(const char* str, bool& data) {
	if( std::strcmp("true", str) == 0 ||
	    std::strcmp("yes",  str) == 0 ||
	    std::strcmp("y",    str) == 0 ||
	    std::strcmp("on",   str) == 0 ) {
		data = true;
		return true;
	} else if(
	    std::strcmp("false", str) == 0 ||
	    std::strcmp("no",    str) == 0 ||
	    std::strcmp("n",     str) == 0 ||
	    std::strcmp("off",   str) == 0 ) {
		data = false;
		return true;
	}
	return false;
}
template <typename T>
bool Numeric(const char* str, T& data) {
	std::istringstream stream(str);
	stream >> data;
	if( stream.fail() || stream.bad() ) { return false; }
	return true;
}
}  // namespace Basic


struct Boolean : Acceptable {
	Boolean(bool& d) : data(d) {}
	unsigned int operator() (int argc, char* argv[]) {
		if( argc > 0 && Basic::Boolean(argv[0], data) ) {
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
struct NumericIMPL : Acceptable {
	NumericIMPL(T& d) : data(d) {}
	unsigned int operator() (int argc, char* argv[]) {
		if( argc < 1 ) { throw LackOfArgumentsError("A number is required"); }
		if( !Basic::Numeric(argv[0], data) ) {
			throw InvalidArgumentError("Unexpected number");
		}
		return 1;
	}
private:
	T& data;
};
template <typename T>
struct NumericIMPL<T> Numeric(T& n) { return NumericIMPL<T>(n); }


}  // namespace Accept
}  // namespace Kazuhiki

#endif /* kazuhiki/basic.h */

