#include "captty.h"

void usage()
{
	std::cout
		<< "\n"
		<< "*Captty (record and play tty session)\n"
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

