/*
 * Kazuhiki
 *
 * Copyright (C) 2008-2009 FURUHASHI Sadayuki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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

template <typename F>
struct Action : Acceptable {
	Action(F f) : function(f) {}
	unsigned int operator() (int argc, char* argv[]) {
		return function(argc, argv);
	}
private:
	F function;
};

}  // namespace IMPL
}  // namespace Kazuhiki

#endif /* kazuhiki/basic_impl.h */

