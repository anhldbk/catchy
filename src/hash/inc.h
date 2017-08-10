/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   xxhash_wrapper.h
 * Author: anhld2
 *
 * Created on July 3, 2017, 9:49 AM
 */

#ifndef HASHINC_H
#define HASHINC_H

#include "xxhash.h"
#include <stdint.h>
uint64_t get_xxhash(const void* data, size_t len, uint64_t seed = 0x7FFFFFFFFFFFFFFF);
uint64_t get_md5hash(const void* data, size_t len, uint64_t seed = 0x7FFFFFFFFFFFFFFF);
uint64_t get_sha1hash(const void* data, size_t len, uint64_t seed = 0x7FFFFFFFFFFFFFFF);

#endif /* HASHINC_H */

