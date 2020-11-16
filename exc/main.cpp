#include "stdafx.h"

import console;

int main(int argc, char **argv) {
	console::writeln(S("Hello modules!"));
	for (auto i = 0; i < argc; ++i) {
		console::writeln(argv[i], strlen(argv[i]));
	}
	return 0;
}