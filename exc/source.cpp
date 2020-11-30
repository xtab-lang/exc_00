#include "pch.h"
#include "source.h"

namespace exy {
int SourceChar::length() const {
	if (pos) {
		auto leadingByte = (int)*pos;
		if ((leadingByte & 0x80) == 0) {
			return 1;
		} if ((leadingByte & 0xE0) == 0xC0) {
			return 2;
		} if ((leadingByte & 0xF0) == 0xE0) {
			return 3;
		} if ((leadingByte & 0xF8) == 0xF0) {
			return 4;
		}
	}
    return 0;
}

bool SourceChar::isValid(int length) const {
	if (pos) {
		auto leadingByte = (int)*pos;
		switch (length) {
			case 1: return (leadingByte & 0x80) == 0;
			case 2: if ((leadingByte & 0xE0) == 0xC0) {
				return (((int)pos[1]) & 0xC0) == 0x80;
			} break;
			case 3: if ((leadingByte & 0xF0) == 0xE0) {
				return ((((int)pos[1]) & 0xC0) == 0x80) && ((((int)pos[2]) & 0xC0) == 0x80);
			} break;
			case 4: if ((leadingByte & 0xF8) == 0xF0) {
				return ((((int)pos[1]) & 0xC0) == 0x80) && ((((int)pos[2]) & 0xC0) == 0x80) &&
					((((int)pos[3]) & 0xC0) == 0x80);
			} break;
			default: {
			} break;
		}
	}
	return false;
}

bool SourceChar::isAlpha() const {
	auto len = length();
	if (len > 1) {
		return true;
	} if (len == 1) {
		auto ch = *pos;
		return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
	}
	return false;
}

bool SourceChar::isDigit() const {
	if (pos) {
		auto ch = *pos;
		return ch >= '0' && ch <= '9';
	}
	return false;
}

bool SourceChar::isAlphaNumeric() const {
	auto len = length();
	if (len > 1) {
		return true;
	} if (len == 1) {
		auto ch = *pos;
		return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9');
	}
	return false;
}

int SourceRange::length() const {
	return (int)(end.pos - start.pos);
}
} // namespace exy