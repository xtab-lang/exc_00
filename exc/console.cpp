////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-20
////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "console.h"

#include <wincon.h>

namespace console {
static SRWLOCK srw{};
static thread_local int locks{};

Locker::Locker() {
	if (++locks == 1) {
		shouldRelease = true;
		AcquireSRWLockExclusive(&srw);
	}
}

Locker::~Locker() {
	if (shouldRelease) {
		ReleaseSRWLockExclusive(&srw);
	}
	--locks;
}

void write(const char *v, int vlen) {
	if (!v || vlen <= 0) {
		return;
	}
	Locker locker{};
	auto handle = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD bytesWritten{};
	WriteConsole(handle, v, vlen, &bytesWritten, nullptr);
}

void write(const char *v) {
	write(v, cstrlen(v));
}

void writeln(const char *v, int vlen) {
	Locker locker{};
	write(v, vlen);
	write(S("\r\n"));
}

void writeln(const char *v) {
	Locker locker{};
	write(v);
	write(S("\r\n"));
}

} // namespace console
