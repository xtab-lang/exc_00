#include "pch.h"

namespace exy {
#define MEM_DEBUG

#if defined(MEM_DEBUG)

constexpr int cap = 0x1000;

struct Block {
	UINT64 id;
	int    size;
};

struct Blocks {
	Block **list{};
	int     length{};

	void initialize() {
		Assert(!list && length == 0);
	}

	void dispose() {
		if (list) {
			for (auto i = 0; i < length; ++i) {
				auto block = list[i];
				if (block) {
					Assert(0);
				}
			}
			HeapFree(GetProcessHeap(), 0, list);
			list = nullptr;
			length = 0;
			Heap::frees++;
		}
	}

	void put(Block *block, int size) {
		if (!list) {
			length = cap;
			list = (Block**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Block*) * length);
			Heap::allocs++;
		} else while (block->id >= length) {
			length *= 2;
			if (list) {
				list = (Block**)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, list, sizeof(Block*) * length);
			}
			Heap::reallocs++;
		} if (list) {
			list[block->id] = block;
		} else {
			Assert(0);
		}
		Heap::used -= block->size;
		Heap::used += size;
		block->size = size;
	}

	void remove(Block *block) {
		Assert(block->id >= 0 && block->id < length);
		list[block->id] = nullptr;
		Heap::used -= block->size;
	}
};

static Blocks blocks{};

struct FreeIds {
	UINT64 *list{};
	int     length{};
	int     capacity{};

	void initialize() {
		Assert(!list && !length && !capacity);
	}

	void dispose() {
		if (list) {
			HeapFree(GetProcessHeap(), 0, list);
			list = nullptr;
			length = 0;
			capacity = 0;
			Heap::frees++;
		}
	}

	UINT64 pop() {
		return list[--length];
	}

	void push(UINT64 id) {
		if (length == capacity) {
			if (capacity) {
				capacity = cap * 2;
				list = (UINT64*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, list, sizeof(int) * capacity);
				Heap::reallocs++;
			} else {
				capacity = cap;
				list = (UINT64*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(int) * capacity);
				Heap::allocs++;
			}
			Heap::used += sizeof(int) * cap;
		} if (list) {
			list[length++] = id;
		} else {
			Assert(0);
		}
	}
};
static FreeIds freeIds{};
#endif

UINT64 Heap::allocs   = 0ui64;
UINT64 Heap::reallocs = 0ui64;
UINT64 Heap::frees    = 0ui64;
UINT64 Heap::used     = 0ui64;
SRWLOCK Heap::srw{};

void Heap::initialize() {
	freeIds.initialize();
	blocks.initialize();
}

void Heap::dispose() {
	blocks.dispose();
	freeIds.dispose();
}

void* Heap::alloc(int size) {
#if defined(MEM_DEBUG)
	auto block = (Block*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(Block) + size);
	if (!block) {
		Assert(0);
		return nullptr;
	}
	AcquireSRWLockExclusive(&srw);
	if (freeIds.length) {
		block->id = freeIds.pop();
	} else {
		block->id = allocs++;
	}
	blocks.put(block, size);
	ReleaseSRWLockExclusive(&srw);
	return (BYTE*)block + sizeof(Block);
#else
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#endif
}

void* Heap::realloc(void *m, int size) {
#if defined(MEM_DEBUG)
	auto block = (Block*)((BYTE*)m - sizeof(Block));
	block = (Block*)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, block, sizeof(Block) + size);
	AcquireSRWLockExclusive(&srw);
	reallocs++;
	blocks.put(block, size);
	ReleaseSRWLockExclusive(&srw);
	return (BYTE*)block + sizeof(Block);
#else
	return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m, size);
#endif
}

void* Heap::free(void *m) {
#if defined(MEM_DEBUG)
	auto block = (Block*)((BYTE*)m - sizeof(Block));
	AcquireSRWLockExclusive(&srw);
	frees++;
	blocks.remove(block);
	freeIds.push(block->id);
	HeapFree(GetProcessHeap(), 0, block);
	ReleaseSRWLockExclusive(&srw);
#else
	HeapFree(GetProcessHeap(), 0, m);
#endif
	return nullptr;
}

void Mem::dispose() {
	if (slabs) {
		for (auto i = 0; i < length; ++i) {
			if (auto slab = slabs[i]) {
				Heap::free(slab);
			}
		}
		Heap::free(slabs);
		slabs = nullptr;
		length = 0;
	}
}

constexpr int slabSize = cap;

void* Mem::doAlloc(int size) {
	Assert(size > 0);
	AcquireSRWLockExclusive(&srw);
	if (!slabs) {
		slabs = (Slab**)Heap::alloc(sizeof(Slab*));
		auto slab = (Slab*)Heap::alloc(sizeof(Slab) + slabSize);
		slab->length = slabSize;
		slabs[length++] = slab;
	}
	void *result{};
	auto slab = slabs[length - 1];
	result = slab->alloc(size);
	if (!result) {
		auto slabLength = slabSize;
		while (slabLength < size + 8) {
			slabLength *= 2;
		}
		slab = (Slab*)Heap::alloc(sizeof(Slab) + slabSize);
		slab->length = slabSize;
		slabs[length++] = slab;
		result = slab->alloc(size);
		Assert(result);
	}
	ReleaseSRWLockExclusive(&srw);
    return nullptr;
}

void* Mem::Slab::alloc(int size) {
	auto unused = length - used;
	if (size <= unused) {
		auto result = (char*)this + used;
		used += size;
		return result;
	}
	return nullptr;
}
} // namespace exy