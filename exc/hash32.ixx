////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-17
////////////////////////////////////////////////////////////////
module;
#include "stdafx.h"
export module hash;

// Forward declarations.
void MurmurHash3_x86_32(const void *key, int len, unsigned int seed, void *out);

export unsigned int hash32(const void *v, int vlen) {
	if (!v || !vlen) {
		return 0ui32;
	}
    unsigned int hash = 0ui32;
    MurmurHash3_x86_32(v, vlen, 0, &hash);
	return hash;
}

// From https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp

//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

__forceinline unsigned int getblock32(const unsigned int *p, int i) {
    return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche
__forceinline unsigned int fmix32(unsigned int h) {
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

//-----------------------------------------------------------------------------
void MurmurHash3_x86_32(const void *key, int len, unsigned int seed, void *out) {
    const unsigned __int8 *data = (const unsigned __int8*)key;
    const int nblocks = len / 4;

    unsigned int h1 = seed;

    const unsigned int c1 = 0xcc9e2d51;
    const unsigned int c2 = 0x1b873593;

    //----------
    // body

    const unsigned int *blocks = (const unsigned int*)(data + nblocks * 4);

    for(int i = -nblocks; i; i++) {
        unsigned int k1 = getblock32(blocks, i);

        k1 *= c1;
        k1 = _rotl(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = _rotl(h1, 13); 
        h1 = h1*5+0xe6546b64;
    }

    //----------
    // tail

    const unsigned __int8 *tail = (const unsigned __int8*)(data + nblocks * 4);

    unsigned int k1 = 0;

    switch(len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
            k1 *= c1; k1 = _rotl(k1, 15); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;

    h1 = fmix32(h1);

    *(unsigned int*)out = h1;
}