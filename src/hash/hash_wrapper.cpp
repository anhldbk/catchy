
/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   xxhash_wrapper.cpp
 * Author: anhld2
 *
 * Created on July 3, 2017, 10:03 AM
 */

#ifndef XXHASH_WRAPPER_CPP
#define XXHASH_WRAPPER_CPP

#include <stdint.h>
#define XXH_STATIC_LINKING_ONLY   /* *_state_t */
#include "md5.h"
#include "sha1.h"
#include "xxhash.h"
#include "inc.h"

uint64_t get_xxhash(const void* data, size_t len, uint64_t seed) {
    XXH64_state_t state;
    uint64_t result;

    result = XXH64(data, len, seed);

    XXH64_reset(&state, seed);
    XXH64_update(&state, data, len);
    result = XXH64_digest(&state);

    return result;
}

uint64_t get_md5hash(const void* data, size_t len, uint64_t seed) {
    uint8_t digest[16];
    md5_state_t state;
    md5_init(&state);

    md5_append(&state, (md5_byte_t *) data, len);
    md5_finish(&state, (md5_byte_t *) & digest);
    uint32_t low =
            (digest[11] << 24 | digest[10] << 16 | digest[9] << 8 | digest[8]);
    uint32_t high =
            (digest[15] << 24 | digest[14] << 16 | digest[13] << 8 | digest[12]);
    uint64_t result;

    result = high;
    result <<= 32;
    result &= 0xffffffff00000000LLU;
    result |= low;

    return result;
}

uint64_t get_sha1hash(const void* data, size_t len, uint64_t seed) {
    SHA1Context sha1_ctx;

    SHA1Reset(&sha1_ctx);
    SHA1Input(&sha1_ctx, (unsigned char*)data, len);
    if (SHA1Result(&sha1_ctx) != 1) {
        return -1;
    }

    uint64_t result = sha1_ctx.Message_Digest[3];
    result <<= 32;
    result |= sha1_ctx.Message_Digest[4];
    return result;
}

#endif /* XXHASH_WRAPPER_CPP */

