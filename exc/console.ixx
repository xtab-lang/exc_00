////////////////////////////////////////////////////////////////
// author: munenedu@gmail.com
//   date: 2020-11-16
////////////////////////////////////////////////////////////////
module;
#include "stdafx.h"
#include <wincon.h>
export module console;

export namespace console {

void write(const char *text, int length) {
	auto out = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD written = 0;
	auto res = WriteConsole(out, text, length, &written, nullptr);
	if (res == FALSE) { // Failed.

	} // Else succeeded.

}

void writeln(const char *text, int length) {
	write(text, length);
	write(S("\r\n"));
}


}