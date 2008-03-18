/*
 * This file is part of Captty.
 *
 * Copyright (C) 2007-2008 FURUHASHI Sadayuki, TOMATSURI Kaname
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

#include "captty.h"
#include <string.h>
#include <stdlib.h>

void usage()
{
	std::cout
		<< "Captty "<<"0.3.0"<<"\n"
		<< "\n"
		<< "  [record tty]$ captty r[ecord] <file>.pty [command...]\n"
		<< "  [replay tty]$ captty p[lay]   <file>.pty\n"
		<< "\n"
		<< "  player keyboard control:\n"
		<< "    g    rewind to start\n"
		<< "    l    skip forward\n"
		<< "    h    skip back\n"
		<< "    k    speed up\n"
		<< "    j    speed down\n"
		<< "    =    reset speed\n"
		<< "    ;    pause/restart\n"
		<< "\n"
		<< std::endl;
}

int main(int argc, char* argv[])
try {
	if( argc < 3 ) {
		usage();
		return 1;
	}
	static const char* record_match[] = {
		"record", "rec", "r",
		"capture", "capt", "cap",
		NULL
	};
	static const char* play_match[] = {
		"play", "pla", "pl", "p",
		NULL
	};
	for(const char** m = record_match; *m != NULL; ++m) {
		if( strcmp(argv[1], *m) == 0 ) {
			if( argc > 3 ) {
				Captty::record(argv[2], &argv[3]);
			} else {
				Captty::record(argv[2], NULL);
			}
			return 0;
		}
	}
	for(const char** m = play_match; *m != NULL; ++m) {
		if( strcmp(argv[1], *m) == 0 ) {
			Captty::play(argv[2]);
			return 0;
		}
	}
	usage();
	return 1;
}
catch (Captty::captty_error& e) {
	std::cout << "error: " << e.what() << std::endl;
	return 1;
}

