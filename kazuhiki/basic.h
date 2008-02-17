#ifndef KAZUHIKI_BASIC_H__
#define KAZUHIKI_BASIC_H__

#include <cstring>
#include "kazuhiki/parser.h"

namespace Kazuhiki {


namespace Convert {

bool Boolean(const char* arg, bool& data) {
	if( std::strcmp("true", arg) == 0 ||
	    std::strcmp("yes",  arg) == 0 ||
	    std::strcmp("y",    arg) == 0 ||
	    std::strcmp("on",   arg) == 0 ) {
		data = true;
		return true;
	} else if(
	    std::strcmp("false", arg) == 0 ||
	    std::strcmp("no",    arg) == 0 ||
	    std::strcmp("n",     arg) == 0 ||
	    std::strcmp("off",   arg) == 0 ) {
		data = false;
		return true;
	}
	return false;
}

template <typename T>
bool Numeric(const char* arg, T& data) {
	std::istringstream stream(arg);
	stream >> data;
	if( stream.fail() || stream.bad() ) { return false; }
	return true;
}

}  // namespace Convert



namespace Accept {

struct Boolean : Acceptable {
	Boolean(bool& d) : data(d) {}
	unsigned int operator() (int argc, char* argv[]) {
		if( argc > 0 && Convert::Boolean(argv[0], data) ) {
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
		if( !Convert::Numeric(argv[0], data) ) {
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

