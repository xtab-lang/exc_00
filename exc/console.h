//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-20
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef CONSOLE_H_
#define CONSOLE_H_

namespace exy {
struct ConsoleStream : FormatStream {
	void lock() override;
	void unlock() override;
	void write(const char *v, int vlen) override;
	virtual void setTextFormat(TextFormat) override;
	virtual void resetTextFormat() override { setTextFormat(TextFormat::Default); }
private:
	static SRWLOCK srw;
	static thread_local int locks;
	static bool initialized;
}; // struct Stream

ConsoleStream getConsoleFormatStream();
} // namespace exy

#endif // CONSOLE_H_