////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-20
////////////////////////////////////////////////////////////////

#include "pch.h"

#include <wincon.h>

namespace exy {
SRWLOCK ConsoleStream::srw{};
thread_local int ConsoleStream::locks = 0;

void ConsoleStream::lock() {
	Assert(locks >= 0);
	if (++locks == 1) {
		AcquireSRWLockExclusive(&srw);
	}
}

void ConsoleStream::unlock() {
	Assert(locks > 0);
	if (--locks == 0) {
		ReleaseSRWLockExclusive(&srw);
	}
}

void ConsoleStream::write(const char *v, int vlen) {
	if (!v || vlen <= 0) {
		return;
	}
	lock();
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD bytesWritten{};
	if (!WriteConsole(handle, v, vlen, &bytesWritten, nullptr)) {
		Assert(0);
	}
	unlock();
}

void ConsoleStream::setTextFormat(TextFormat color) {
	// Syntax: '0x1b[' value 'm'
#define ESC "\033["
#define ESC_LEN cstrlen(ESC)
	char *buf = numbuf;
	memcopy(buf, ESC, ESC_LEN);
	_itoa_s((int)color, (char*)buf + ESC_LEN, numbufcap, 10);
	auto length = cstrlen(buf);
	buf[length++] = 'm';
	write(buf, length);
}

ConsoleStream getConsoleFormatStream() {
	return {};
}
} // namespace exy
