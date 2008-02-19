#ifndef KAZUHIKI_BASIC_IMPL_H__
#define KAZUHIKI_BASIC_IMPL_H__

namespace Kazuhiki {
namespace IMPL {

bool ConvertBoolean(const char* arg, bool& data) {
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
bool ConvertNumeric(const char* arg, T& data) {
	std::istringstream stream(arg);
	stream >> data;
	if( stream.fail() || stream.bad() ) { return false; }
	return true;
}

template <typename T>
struct NumericIMPL : Acceptable {
	NumericIMPL(T& d) : data(d) {}
	unsigned int operator() (int argc, char* argv[]) {
		if( argc < 1 ) { throw LackOfArgumentsError("A number is required"); }
		if( !IMPL::ConvertNumeric(argv[0], data) ) {
			throw InvalidArgumentError("Unexpected number");
		}
		return 1;
	}
private:
	T& data;
};

}  // namespace IMPL
}  // namespace Kazuhiki

#endif /* kazuhiki/basic_impl.h */

