#include "pch.h"

namespace exy {
#define MEM_DEBUG

#if defined(MEM_DEBUG)

constexpr int cap = 0x1000;

template<typename T>
static T* heapAlloc(int count) {
	auto size = sizeof(T) * count;
	auto    m = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	Heap::allocs++;
	Heap::used += size;
	return (T*)m;
}

template<typename T>
static T* heapRealloc(T *m, int newCount, int oldCount) {
	auto newSize = sizeof(T) * newCount;
	auto oldSize = sizeof(T) * oldCount;
	auto       x = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m, newSize);
	Heap::reallocs++;
	Heap::used -= oldSize;
	Heap::used += newSize;
	return (T*)x;
}

template<typename T>
static T* heapFree(T *m, int count) {
	HeapFree(GetProcessHeap(), 0, m);
	Heap::frees++;
	Heap::used -= sizeof(T) * count;
	return (T*)nullptr;
}

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
			list = heapFree(list, length);
			length = 0;
		}
	}

	void put(Block *block, int size) {
		if (!list) {
			length = cap;
			list = heapAlloc<Block*>(length);
		} else while (block->id >= length) {
			auto oldLength = length;
			length *= 2;
			list = heapRealloc(list, length, oldLength);
		}
		//Assert(block->id != 72);
		Assert(block->id < length);
		list[block->id] = block;
		block->size = size;
	}

	void remove(Block *block) {
		Assert(block->id >= 0 && block->id < length);
		list[block->id] = nullptr;
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
			list     = heapFree(list, capacity);
			length   = 0;
			capacity = 0;
		}
	}

	UINT64 pop() {
		return list[--length];
	}

	void push(UINT64 id) {
		if (length == capacity) {
			if (capacity) {
				auto oldcap = capacity;
				capacity = cap * 2;
				list = heapRealloc(list, capacity, oldcap);
			} else {
				capacity = cap;
				list = heapAlloc<UINT64>(capacity);
			}
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
	Assert(allocs == frees);
	Assert(used == 0);
}

void* Heap::alloc(int size) {
	if (size <= 0) {
		return nullptr;
	}
#if defined(MEM_DEBUG)
	AcquireSRWLockExclusive(&srw);
	auto block = (Block*)heapAlloc<char>(sizeof(Block) + size);
	if (freeIds.length) {
		block->id = freeIds.pop();
	} else {
		block->id = Heap::allocs - 1;
	}
	blocks.put(block, size);
	ReleaseSRWLockExclusive(&srw);
	return (char*)block + sizeof(Block);
#else
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#endif
}

void* Heap::realloc(void *m, int size) {
#if defined(MEM_DEBUG)
	if (!m) {
		return Heap::alloc(size);
	} if (size <= 0) {
		return nullptr;
	}
	constexpr int sizeOfBlock = sizeof(Block);
	auto block = (Block*)((char*)m - sizeOfBlock);
	AcquireSRWLockExclusive(&srw);
	block = (Block*)heapRealloc<char>((char*)block, sizeOfBlock + size, sizeOfBlock + block->size);
	blocks.put(block, size);
	ReleaseSRWLockExclusive(&srw);
	return (char*)block + sizeof(Block);
#else
	if (size <= 0) {
		return nullptr;
	} if (m) {
		return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m, size);
	}
	return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
#endif
}

void* Heap::free(void *m) {
	if (m) {
	#if defined(MEM_DEBUG)
		auto block = (Block*)((char*)m - sizeof(Block));
		AcquireSRWLockExclusive(&srw);
		blocks.remove(block);
		freeIds.push(block->id);
		heapFree((char*)block, sizeof(Block) + block->size);
		ReleaseSRWLockExclusive(&srw);
	#else
		HeapFree(GetProcessHeap(), 0, m);
	#endif
	}
	return nullptr;
}

void Mem::dispose() {
	if (slabs) {
		for (auto i = 0; i < length; ++i) {
			if (auto slab = slabs[i]) {
				heapFree((char*)slab, slab->length);
			}
		}
		slabs = heapFree(slabs, length);
		length = 0;
	}
}

constexpr auto defaultSlabSize = cap;

static auto getSlabSize(int dataSize) {
	auto size = defaultSlabSize;
	while (size < dataSize) size *= 2;
	return size;
}

void* Mem::doAlloc(int size) {
	Assert(size > 0);
	AcquireSRWLockExclusive(&srw);
	void *result{};
	if (!slabs) {
		auto slabSize = getSlabSize(size + sizeof(Slab));
		auto     slab = (Slab*)heapAlloc<char>(slabSize);
		slab->length  = slabSize;
		slab->used    = sizeof(Slab);

		length = 1;
		slabs  = heapAlloc<Slab*>(length);
		slabs[length - 1] = slab;

		result = slab->alloc(size);
	} else {
		auto slab = slabs[length - 1];
		result    = slab->alloc(size);
		if (!result) {
			auto slabSize = getSlabSize(size + sizeof(Slab));
			slab          = (Slab*)heapAlloc<char>(slabSize);
			slab->length  = slabSize;
			slab->used    = sizeof(Slab);

			++length;
			slabs = heapRealloc(slabs, length, length - 1);
			slabs[length - 1] = slab;

			result = slab->alloc(size);
		}
	}
	ReleaseSRWLockExclusive(&srw);
	Assert(result);
    return result;
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