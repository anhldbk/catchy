/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   common.h
 * Author: anhld2
 *
 * Created on June 14, 2017, 9:50 AM
 */

#ifndef JUMPTABLE_H
#define JUMPTABLE_H

#include <stdint.h>
#include <vector>
#include <exception>
#include <atomic>
#include <stdexcept>
#include <string.h>
#include <iostream>
using namespace std;

#define NULL_INDEX          0
#define JT_VERSION           1
// Maximum number of entries (default = 2^20)
#define MAX_INT20           1048576


template<typename T>
struct JumpEntry {
    uint32_t next; // link the the next entry
    T data;

    uint32_t get_entry_size() {
        return sizeof (JumpEntry);
    }
};

// Schema of JumpTable: <header><entries>
// Currently, only one tail is used.

template<typename T>
struct JumpTable {
    // header
    uint32_t version : 16; // currently version 1
    uint32_t header_size : 16; // size of header in byte

    uint32_t entry_capacity; // maximum number of entries
    atomic<uint32_t> entry_avail; // number of available entries
    atomic<uint32_t> last_free_entry; // index of the last free head

    // i(th) head is free if bit i(th) of head_state is clear to 0
    atomic<uint8_t> state[(MAX_INT20 >> 3) + 1];

    /**
     * Constructor
     * Warning: If you invoke this method to create instances, there's no
     *  data associated for `head` and `tail`
     */
    JumpTable() {
        this->version = JT_VERSION;
        this->header_size = static_cast<uint16_t> (sizeof (JumpTable));
        this->last_free_entry = 0;
        this->entry_capacity = 0;
        this->entry_avail = 0;
    }

    /**
     * Create a new instance of JumpTable.
     * All related data areas will be initialized as well.
     * @param num_entries Number of available entries. Default is 0 and we
     *      will allocate a preconfigured number of entries
     * @return Pointer to the newly created instance
     */
    static JumpTable* create_ptr(uint32_t num_entries = 0) {
        uint32_t size = JumpTable<T>::calculate_size(num_entries);
        uint8_t* buffer = new uint8_t[size]();
        return JumpTable<T>::init_ptr(buffer, num_entries);
    }

    /**
     * Release pointers created by `create_ptr`
     * @param ptr   Pointer to an instance of JumpTable
     */
    static void release_ptr(JumpTable* ptr) {
        delete[] (uint8_t*) ptr;
    }

    /**
     * Initializes a pre-allocated buffer and returns an instance of JumpTable
     * Buffers can be allocated by using `create_ptr` or 
     * @param ptr   Pointer to an instance of JumpTable
     */
    static JumpTable* init_ptr(uint8_t* buffer, uint32_t num_entries = 0) throw (invalid_argument) {
        if (buffer == nullptr) {
            throw invalid_argument("Invalid buffer");
        }        
        // if `num_entries` = 0, we set its value to the maximum one
        num_entries = (num_entries == 0 || num_entries > MAX_INT20) ? MAX_INT20 : num_entries;

        JumpTable* jt_ptr = (JumpTable*) buffer;

        // initialize fields
        jt_ptr->version = JT_VERSION;
        jt_ptr->header_size = static_cast<uint16_t> (sizeof (JumpTable<T>));
        jt_ptr->last_free_entry = 1; // entry 0 is reserved as NULL entries
        jt_ptr->entry_capacity = num_entries    ;
        jt_ptr->entry_avail = num_entries - 1;

        jt_ptr->clear();

        return jt_ptr;
    }

    static JumpTable* from_ptr(uint8_t* buffer) throw (invalid_argument) {
        if (buffer == nullptr) {
            throw invalid_argument("Invalid buffer");
        }
        JumpTable* jt_ptr = (JumpTable*) buffer;
        if (jt_ptr->version != JT_VERSION) {
            throw invalid_argument("Invalid version");
        }
        return jt_ptr;
    }

    /**
     * Assuming that there's no concurrent call to this method
     */
    void clear() {
        this->last_free_entry = 1;
        this->entry_avail = this->entry_capacity - 1;

        // mark all heads free
        uint32_t state_size = (entry_capacity >> 3) + 1;
        memset(this->state, 0, state_size);
    }

    /**
     * Check if the table is full or not
     * @return If there's no head or no tail left, returns true.
     *      Otherwise, returns false
     */
    bool is_full() {
        return this->entry_avail == 0;
    }

    uint8_t* get_ptr(uint32_t offset = 0) {
        return (uint8_t*)this +offset;
    }

    /**
     * Get address of the 1st entry
     * @return Pointer to the entry
     */
    uint8_t* get_entry_section() {
        uint32_t offset = this->header_size;
        offset += (entry_capacity >> 3) + 1; // entry states
        return this->get_ptr(offset);
    }

    void _update_entry_avail(uint32_t offset, bool increase = true) {
        uint32_t expected, desired;
        bool exchanged;

        do {
            expected = entry_avail;
            if (increase) {
                desired = expected + offset;
            } else {
                desired = expected - offset;
            }
            exchanged = entry_avail.compare_exchange_strong(expected, desired);
        } while (!exchanged);
    }

    void _update_last_free_entry(uint32_t index) {
        uint32_t expected;
        bool exchanged = false;

        // modify last_free_entry
        do {
            expected = last_free_entry;
            exchanged = last_free_entry.compare_exchange_strong(expected, index);
        } while (!exchanged);
    }

    /**
     * Get free entries. JumpEntrys are chained via `next` fields.
     *
     * @param count Number of entries to reserve. Minimum value is 1.
     * @return Index of the first entry.
     */
    uint32_t reserve_entries(uint16_t count = 1) throw (invalid_argument, runtime_error) {
        uint32_t index;
        vector<uint32_t> entries;
        bool exchanged, is_free;
        atomic<uint8_t>* bucket_ptr;
        uint8_t expected, desired, pos;
        if (count == 0) {
            throw invalid_argument("Count must be non zero");
        }
        if (entry_avail < count) {
            // may be we run out of free heads
            throw runtime_error("No available free entries");
        }

        // modify entry_avail first
        _update_entry_avail(count, false);

        // beware of concurrent modifications

        for (index = last_free_entry; index < entry_capacity; index++) {
            bucket_ptr = &this->state[index >> 3];
            expected = bucket_ptr->load();
            pos = index % 8;

            is_free = ((expected >> pos) & 1) == 0;
            if (!is_free) {
                continue;
            }

            desired = expected | (1 << pos);
            exchanged = bucket_ptr->compare_exchange_strong(expected, desired);
            if (!exchanged) { // we have concurrent modifications
                continue;
            }

            entries.push_back(index);

            count -= 1;
            if (count == 0) {
                break;
            }
        }


        if (entries.size() == 0) {
            throw runtime_error("No available free entries");
        }

        _update_last_free_entry(index + 1);

        // chain entries
        index = entries.size() - 1;
        get_entry(entries[index])->next = NULL_INDEX;
        while (index > 0) {
            index -= 1;
            get_entry(entries[index])->next = entries[index + 1];
        }

        return entries[0];
    }

    /**
     * Check if a head is free via its index
     * @param index The index
     * @return If the head is free, returns true. Otherwise, returns false.
     */
    bool is_free_entry(uint32_t index) {
        atomic<uint8_t>* bucket_ptr = &this->state[index >> 3];
        return ((bucket_ptr->load() >> (index % 8)) & 1) == 0;
    }

    /**
     * Free a single reserved entries
     * @param entry_index    Index of the reserved entry
     */
    void _free_entry(uint32_t entry_index) {
        atomic<uint8_t>* bucket_ptr = &this->state[entry_index >> 3];
        uint8_t pos = entry_index % 8;
        uint8_t expected, desired;
        bool exchanged = false, is_free;

        // update state
        while (!exchanged) {
            expected = bucket_ptr->load();
            is_free = !((expected >> pos) & 1);
            if (is_free) {
                break;
            }
            desired &= ~(1 << pos);
            exchanged = bucket_ptr->compare_exchange_strong(expected, desired);
        }

        _update_last_free_entry(entry_index);
        _update_entry_avail(1);
    }

    /**
     * Free a chain of reserved entries
     * @param entry_index   Index of the first entry
     */
    void free_entries(uint32_t entry_index) {
        uint32_t next_index, min_index = entry_index;

        if (entry_index == 0) {
            return; // nothing to do
        }
        while (entry_index != NULL_INDEX) {
            if (min_index > entry_index) {
                min_index = entry_index;
            }
            next_index = get_entry(entry_index)->next;
            _free_entry(entry_index);
            entry_index = next_index;
        }
        _update_last_free_entry(min_index);
    }

    /**
     * Get the entry at a specified index
     * @param tail_index    The index
     * @return The entry. If there's no such index, returns null.
     */
    JumpEntry<T>* get_entry(uint32_t entry_index) {
        if (entry_index >= this->entry_capacity || entry_index == 0) {
            return nullptr;
        }
        JumpEntry<T>* entry_ptr = (JumpEntry<T>*) this->get_entry_section();
        return entry_ptr + entry_index;
    }

    /**
     * Get this table's size (in bytes)
     * @return The total size
     */
    uint32_t get_size() {
        return JumpTable<T>::calculate_size(this->entry_capacity);
    }

    static uint32_t calculate_size(uint32_t jump_capacity) throw(invalid_argument){
        if(jump_capacity > MAX_INT20) {
            throw invalid_argument("Maximum number of entries allowed is MAX_INT20");
        }
        uint32_t size = sizeof (JumpTable<T>); // header
        size += jump_capacity * sizeof (JumpEntry<T>); // entries        
        return size;
    }

    /**
     * Get the number of available entries
     * @return  The number
     */
    uint32_t get_avail_entries() {
        return entry_avail;
    }
};

#endif /* JUMPTABLE_H */
