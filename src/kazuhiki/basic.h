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
	
	struct Character : Acceptable {
		Character(char& d) : data(d) {}
		unsigned int operator() (int argc, char* argv[]) {
			if( argc < 1 ) { throw LackOfArgumentsError("A character is required"); }
			if( argv[0][0] != '\0' && argv[0][1] == '\0' ) {
				data = argv[0][0];
				return 1;
			} else {
				throw InvalidArgumentError("One character is expected");
			}
		}
	private:
		char& data;
		Character();
	};
	
	template <typename T>
	IMPL::NumericIMPL<T> Numeric(T& n) { return IMPL::NumericIMPL<T>(n); }

	template <typename F>
	IMPL::Action<F> Action(F function) { return IMPL::Action<F>(function); }
}


namespace Convert {

	void Boolean(char* arg, bool& d) {
		Accept::Boolean f(d);
		f(1, &arg);
	}

	void String(char* arg, std::string& d) {
		d = arg;
	}

	void Character(char* arg, char& d) {
		Accept::Character f(d);
		f(1, &arg);
	}

	template <typename T>
	void Numeric(char* arg, T& n) {
		IMPL::NumericIMPL<T> f(n);
		f(1, &arg);
	}
}


}  // namespace Kazuhiki

#endif /* kazuhiki/basic.h */

