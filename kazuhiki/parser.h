#ifndef KAZUHIKI_PARSER_H__
#define KAZUHIKI_PARSER_H__

#include <string>
#include <iostream>
#include <map>
#include <utility>
#include <stdexcept>
#include <cstring>
#include <sstream>


namespace Kazuhiki {


struct Acceptable {
	virtual ~Acceptable() {}
	virtual unsigned int operator() (int argc, char* argv[]) = 0;
};


class AcceptableMap {
public:
	~AcceptableMap()
	{
		for(map_t::iterator it(map.begin()), it_end(map.end());
				it != it_end;
				++it ) {
			delete it->second.accept;
		}
	}
	void set(const std::string& key, Acceptable* base, bool* notify)
	{
		map_t::iterator a( map.find(key) );
		if( a == map.end() ) {
			map.insert( std::make_pair(key, item_t(base,notify)) );
		} else {
			delete a->second.accept;
			a->second.accept = base;
			a->second.notify = notify;
		}
	}
	Acceptable* search(const std::string& key)
	{
		map_t::iterator a( map.find(key) );
		if( a == map.end() ) {
			return NULL;
		} else {
			if( a->second.notify ) { *a->second.notify = true; }
			return a->second.accept;
		}
	}
private:
	struct item_t {
		item_t(Acceptable* a, bool* n) : accept(a), notify(n) {}
		Acceptable* accept;
		bool* notify;
	};
	typedef std::map<std::string, item_t> map_t;
	map_t map;
};


struct ArgumentError : public std::runtime_error {
	ArgumentError(const std::string& msg) : std::runtime_error(msg) {}
	~ArgumentError() throw() {}
	void setkey(const std::string& k) throw() { msg = std::string("`") + k + "': " + what(); }
	virtual const char* what() const throw() {
		if( msg.empty() ) {
			return std::runtime_error::what();
		} else {
			return msg.c_str();
		}
	}
private:
	std::string msg;
};

struct UnknownArgumentError : ArgumentError {
	UnknownArgumentError(const std::string& name) :
		ArgumentError("Unknown argument") {}
};

struct LackOfArgumentsError : ArgumentError {
	LackOfArgumentsError(const std::string& msg) :
		ArgumentError(msg) {}
};

struct InvalidArgumentError : ArgumentError {
	InvalidArgumentError(const std::string& msg) :
		ArgumentError(msg) {}
};

class Parser {
public:
	Parser() {}
public:
	template <typename Accept>
	void on( const std::string& short_name,
		 const std::string& long_name,
		 Accept ac, bool& notify)
	{
		notify = false;
		if(!short_name.empty()) {
			map.set(short_name, new Accept(ac), &notify);
		}
		if(!long_name.empty()) {
			map.set(long_name, new Accept(ac), &notify);
		}
	}
	template <typename Accept>
	void on( const std::string& short_name,
		 const std::string& long_name,
		 Accept ac)
	{
		if(!short_name.empty()) {
			map.set(short_name, new Accept(ac), NULL);
		}
		if(!long_name.empty()) {
			map.set(long_name, new Accept(ac), NULL);
		}
	}
	void parse(int argc, char* argv[])
	{
		parse_base<&Parser::parse_through, &Parser::fail_through>(argc, argv);
	}
	void break_parse(int& argc, char* argv[])
	{
		parse_base<&Parser::parse_break, &Parser::fail_through>(argc, argv);
	}
	void order(int argc, char* argv[])
	{
		parse_base<&Parser::parse_through, &Parser::fail_exception>(argc, argv);
	}
	void break_order(int& argc, char* argv[])
	{
		parse_base<&Parser::parse_break, &Parser::fail_exception>(argc, argv);
	}
private:
	template < int (Parser::*parse_method)(int& argc, char* argv[], int i, int s),
		   int (Parser::*fail_method)(int& argc, char* argv[], int i) >
	void parse_base(int& argc, char* argv[])
	{
		int i = 0;
		while(i < argc) {
			std::string key(argv[i]);
			try {
				Acceptable* as = map.search(key);
				if(as) {
					int s = (*as)(argc - i - 1, argv + i + 1);
					i = (this->*parse_method)(argc, argv, i, s + 1);
				} else {
					i = (this->*fail_method)(argc, argv, i);
				}
			} catch (ArgumentError& e) {
				e.setkey(key);
				throw;
			}
		}
	}
private:
	int parse_through(int& argc, char* argv[], int i, int s)
	{
		return i + s;
	}
	int parse_break(int& argc, char* argv[], int i, int s)
	{
		memcpy(argv + i, argv + i + s, (argc - i - s)*sizeof(char*));
		argc -= s;
		return i;
	}
	int fail_through(int& argc, char* argv[], int i)
	{
		return i + 1;
	}
	int fail_exception(int& argc, char* argv[], int i)
	{
		throw UnknownArgumentError(argv[i]);
	}
private:
	AcceptableMap map;
};


}  // namespace Kazuhiki

#endif /* kazuhiki/parser.h */

