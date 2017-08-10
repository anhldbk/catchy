/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FragmentEntry.h
 * Author: anhld2
 *
 * Created on June 20, 2017, 11:38 AM
 */

#ifndef FRAGMENTENTRY_H
#define FRAGMENTENTRY_H

#include <stdint.h>
#include <exception>
#include <atomic>
using namespace std;

#define NULL_INDEX          0

struct ObjectEntry {
    uint32_t region_head : 24; // 24 bit wide
    uint32_t pinned : 1;
    uint32_t extra : 7;

    uint32_t jump_head : 20; // 20 bit wide

    ObjectEntry() {
        this->region_head = 0;
        this->pinned = 0;
        this->extra = 0;
        this->jump_head = 0;
    }

    bool has_no_jumps() {
        return this->jump_head == 0;
    }
};

struct NameEntry {
    // associated object's key (little endian)
    uint32_t object_key[2];
};

// 14 bytes in size
struct FragmentEntry {
    // type = 0 => this is a free entry
    // type = 0xF ==> this is a name entry
    // type = 0xE ==> this is a gonna-be-used entry
    // type [1 => 12] indicates fragment sizes
    // fragment_size = 2^(9 + i) to align to disk sectors (512 = 2^9bytes)
    // fragment_size must fall into the range of [2^10, 2^21]
    // <=> [1kb, 2kb, 4kb, 8kb, 16kb, 32kb, 64kb, 128kb, 256kb, 512kb, 1mb, 2mb]
    // uint32_t type : 4;
    // for search
    // uint32_t tag : 28;
    atomic<uint32_t> type_tag;

    // for hash collisions
    atomic<uint16_t> next;

    union {
        ObjectEntry object;
        NameEntry name;
    };

    FragmentEntry();

    bool is_object_entry();

    /**
     * Get the object key stored in a name entry
     * @return Returns the object key if success. 
     *   Otherwise, throws errors if the entry is not a name one.
     */
    uint64_t get_object_key() throw (runtime_error);

    /**
     * Check if this fragment is free
     * If it is, set field `type` to 0xE (gonna be used)
     * @param acquire If the entry is free and `acquire` is set to `true`, 
     *      the entry will be acquired.
     * @return Returns true if the fragment is free. Otherwise, returns false.
     */
    bool is_free(bool acquire = true);

    /**
     * Check if this fragment is a tail
     * @return Returns true if the fragment is. Otherwise, returns false.
     */
    bool is_tail();

    void free();

    uint8_t get_type();

    /**
     * Set the fragment's type
     * @param type  Type. Must in the range of [0, 0xF]      
     * @return Returns true if the type is set. Otherwise, returns false.
     */
    bool set_type(uint32_t type) throw (invalid_argument);

    uint32_t get_tag();

    /**
     * Set the fragment's tag
     * @param Tag  12-bit tag.     
     * @return Returns true if the tag is set. Otherwise, returns false.
     */
    bool set_tag(uint32_t tag);

    /**
     * Set the fragment's next
     * @param next  Index of the next entry.     
     * @return Returns true if `next` is set. Otherwise, returns false.
     */
    bool set_next(uint16_t next);

    uint16_t get_next();
};

uint64_t FragmentEntry::get_object_key() throw (runtime_error) {
    if (this->is_object_entry()) {
        throw runtime_error("Invoke this method on a name entry only");
    }
    uint64_t key = this->name.object_key[0];
    key = this->name.object_key[1] | (key << 32);
    return key;
}

bool FragmentEntry::set_next(uint16_t next) {
    uint16_t expected = this->next;
    return this->next.compare_exchange_strong(expected, next);
}

uint16_t FragmentEntry::get_next() {
    return this->next;
}

FragmentEntry::FragmentEntry() {
    this->type_tag = 0;
    this->next = 0;
}

bool FragmentEntry::is_object_entry() {
    return this->get_type() != 0xF;
}

bool FragmentEntry::is_tail() {
    return this->next == NULL_INDEX;
}

bool FragmentEntry::is_free(bool acquire) {
    if (!acquire) {
        return (this->type_tag >> 28) == 0;
    }

    uint32_t expected = this->type_tag & 0x0FFFFFFF;
    uint32_t desired = (this->type_tag & 0x0FFFFFFF) | 0xE0000000;
    bool exchanged = this->type_tag.compare_exchange_strong(expected, desired);
    // if we can't exchange, may be there's another thread acquire this fragment
    return exchanged;
}

void FragmentEntry::free() {
    uint32_t expected = this->type_tag;
    uint32_t desired = this->type_tag & 0x0FFFFFFF;
    this->type_tag.compare_exchange_strong(expected, desired);
}

uint8_t FragmentEntry::get_type() {
    return this->type_tag >> 28;
}

bool FragmentEntry::set_type(uint32_t type) throw (invalid_argument) {
    if (type > 0xF) {
        throw invalid_argument("Invalid type. Must be in the range of [0, 0xF].");
    }
    uint32_t expected = this->type_tag;
    uint32_t desired = (type << 28) | (expected & 0x0FFFFFFF);
    return this->type_tag.compare_exchange_strong(expected, desired);
}

uint32_t FragmentEntry::get_tag() {
    return this->type_tag & 0x0FFFFFFF;
}

bool FragmentEntry::set_tag(uint32_t tag) {
    uint32_t expected = this->type_tag;
    uint32_t desired = expected & 0xF0000000 | (tag & 0x0FFFFFFF);
    return this->type_tag.compare_exchange_strong(expected, desired);
}

#endif /* FRAGMENTENTRY_H */

