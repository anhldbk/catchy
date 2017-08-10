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

#ifndef FRAGMENTTABLE_H
#define FRAGMENTTABLE_H

#include "FragmentEntry.h"
#include "JumpTable.h"
#include "utils.h"
#include <fcntl.h>
#include "common.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <vector>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <atomic>

using namespace std;

#define MAX_INT24 16777216
#define MAX_INT20 1048576
#define MAX_INT16 65536
#define NULL_INDEX 0
#define FT_VERSION 1
#define FREE_ENTRY 0

// JumpEntry will be 16 bytes in size
// Jumping distance is a 16-bit wide number (+/- 65536)
typedef uint16_t JmpDistance;
typedef JmpDistance JmpEntryData[6];
typedef JumpEntry<JmpEntryData> JmpEntry;
typedef JumpTable<JmpEntryData> JmpTable;

// Schema of FragmentTable: <header><head><tail><JmpTable>
// currently, with 20bit head and 16bit tail, the table's memory footprint
// is about 12mb

struct FragmentTable {
    // Header
    uint32_t version : 16; // currently 1
    uint32_t header_size : 16; // size of header in byte

    // number of entries, currently 2^20
    uint32_t head_capacity;
    // number of entries, currently 2^16
    uint32_t tail_capacity;
    // non-zero index of the first entry where our tail begins
    atomic<uint32_t> tail_first;
    // non-zero index of the last entry where our tail ends
    atomic<uint32_t> tail_last;
    // number of available entries
    atomic<uint32_t> tail_avail;

    // Information about mmaped area
    // Descriptor of the associated region file
    int32_t fd;
    size_t map_offset;

    /**
     * Create a new instance of FragmentTable.
     * All related data areas will be initialized as well.
     * @param head_capacity Number of available entries in Head. 0 means MAX_INT20
     * @param tail_capacity Number of entries in Tail. 0 means MAX_INT16
     * @param jump_capacity Number of entries in JmpTable. 0 means MAX_INT20
     * @return Pointer to the newly created instance
     */
    static FragmentTable * create_ptr(uint32_t head_capacity = 0, uint32_t tail_capacity = 0, uint32_t jump_capacity = 0) throw (invalid_argument);

    /**
     * Get a FragmentTable pointer from meta information
     * If the associated file does NOT exist, it will be created and initialized.
     * If it does exist, it will be loaded instead.
     * @param meta  Meta information about tables
     * @return A FragmentTable pointer.
     */
    static FragmentTable* load(const TableMeta& meta) throw (invalid_argument, runtime_error);
    
    uint32_t get_fragment_index(uint64_t object_key, uint32_t jump_index) throw (invalid_argument, runtime_error);

    vector<uint32_t> get_fragment_indices(FragmentEntry *object_entry_ptr);

    vector<uint32_t> get_fragment_indices(uint64_t object_key) throw (invalid_argument);    
    
    void update_jump_entry(uint64_t object_key, uint32_t jump_index, JmpDistance jump_value) throw (invalid_argument, runtime_error);

    void update_region_head(uint64_t object_key, uint32_t region_head) throw (invalid_argument, runtime_error);
    
    /**
     * Create a new instance of FragmentTable and store it into a file
     * All related data areas will be initialized as well.
     * @param  file_path   A valid path of a wrtiable file.          * @param
     * @param head_capacity Number of available entries in Head. 0 means MAX_INT20
     * @param tail_capacity Number of entries in Tail. 0 means MAX_INT16
     * @param jump_capacity Number of entries in JmpTable. 0 means MAX_INT20
     * @return Pointer to the newly created instance
     */
    static FragmentTable * create_ptr(const char *file_path, uint32_t head_capacity = 0, uint32_t tail_capacity = 0, uint32_t jump_capacity = 0) throw (invalid_argument);

    /**
     * Create a new FragmentTable pointer form a FragmentTable-serialized data
     * @return The pointer
     */
    static FragmentTable *from_ptr(uint8_t *buffer) throw (invalid_argument);

    /**
     * Create a new FragmentTable pointer form a FragmentTable-serialized file
     * @param  file_path   A valid path of a readable file.
     * @return The pointer
     */
    static FragmentTable *from_file(const char *file_path) throw (invalid_argument, runtime_error);

    /**
     * Initialize a memory buffer with specific capacities.
     * Note: The buffer must be allocated with enough memory. You can use `calculate_size()` 
     *      to know how many bytes the buffer should use.
     * @param buffer The buffer
     * @param head_capacity Number of available entries in Head. 0 means MAX_INT20
     * @param tail_capacity Number of entries in Tail. 0 means MAX_INT16
     * @param jump_capacity Number of entries in JmpTable. 0 means MAX_INT20
     * @return Pointer to an initialized FragmentTable instance (having the same address as `buffer`)
     */
    static FragmentTable * init_ptr(uint8_t *buffer, uint32_t head_capacity = 0, uint32_t tail_capacity = 0, uint32_t jump_capacity = 0) throw (invalid_argument);

    /**
     * Release pointers created by `create_ptr`
     * @param ptr   Pointer to an instance of JumpTable
     */
    static void release_ptr(FragmentTable *ptr);

    void release();

    /**
     * Get this table's size (in bytes)
     * @return The total size
     */
    uint32_t get_size();

    static uint32_t calculate_size(uint32_t head_capacity = 0, uint32_t tail_capacity = 0, uint32_t jump_capacity = 0) throw (invalid_argument);

    /**
     * Reverse entries
     * @param  object_key       Object keys
     * @param  fragment_type    Fragment type
     * @param  region_head      Region head
     * @param  jmps             Array of relative distances between consecutive
     * fragments
     * @return       The reserved fragment entry
     */
    FragmentEntry *reserve_entry(uint64_t object_key, uint16_t fragment_type, uint32_t region_head, vector<JmpDistance> jmps) throw (invalid_argument, runtime_error);

    FragmentEntry *reserve_entry(uint64_t name_key, uint64_t object_key) throw (invalid_argument, runtime_error);

    /**
     * Obtain an entry for a specific key
     * @param  key A key
     * @return Returns the obtained entry if success. Otherwise, throw runtime
     * errors
     */
    FragmentEntry *_acquire_entry(uint64_t key) throw (runtime_error);

    /**
     * Obtain jumping steps stored in an object entry
     * @param  object_entry_ptr A pointer to an object entry
     * @return Returns an array of jumping steps if success.
     */
    vector<JmpDistance> get_jumps(FragmentEntry *object_entry_ptr) throw (invalid_argument);

    /**
     * Free reserved entries
     * Note that: 
     * + We're also free associated Jump entries and Fragments
     * + If you want to free keys associated with a specific object (name & object keys), 
     *      you must invoke `free_entry` with individual keys respectively.
     * @param  key A name key or an object key
     * @param  fragment_indices Indices of associated fragments to free
     * @return Returns true if success. Otherwise, returns false.
     */
    bool free_entry(uint64_t key, vector<uint32_t>& fragment_indices);

    bool free_entry(uint64_t key);

    /**
     * Get the entry associated with a key
     * @param  key The key. May be an object or a name key
     * @return Returns the entry if the key is found. Otherwise, returns nullptr.
     */
    FragmentEntry *get_entry(uint64_t key);

    /**
     * Get the object entry associated with a key
     * @param  key The key. May be an object or a name key
     * @return Returns the entry if the key is found. Otherwise, returns nullptr.
     */
    FragmentEntry *get_object_entry(uint64_t key);

    uint8_t *get_ptr(uint32_t offset = 0);

    uint8_t *get_head_section();

    uint8_t *get_tail_section();

    uint8_t *get_jump_section();

    JmpTable *get_jump_table();

    void _update_tail_avail(bool increase = true) throw (runtime_error);

    /**
     * Update `tail_last` to point to a new index, also increase `tail_avail` by 1
     * @param  tail_next The index
     */
    void _update_tail_last(uint32_t tail_next);

    FragmentEntry *_get_head_entry(uint32_t head_index) throw (invalid_argument);

    /**
     * Get the tail entry at a specified index
     * @param  tail_index The index
     * @return Returns the entry if the index is valid. Otherwise, returns nullptr
     * or throws exceptions.
     */
    FragmentEntry *_get_tail_entry(uint32_t tail_index) throw (invalid_argument);

    void clear();

    uint32_t _reserve_tail_entry() throw (runtime_error);

    static void _validate_capacities(uint32_t &head_capacity, uint32_t &tail_capacity, uint32_t &jump_capacity) throw (invalid_argument);

    static void _handle_runtime_error(const char* error) throw (runtime_error);
};

void FragmentTable::release(){
    this->release_ptr(this);
}

FragmentTable* FragmentTable::load(const TableMeta& meta) throw (invalid_argument, runtime_error) {
    FragmentTable* fragment_tbl_ptr = nullptr;
    if (utils::file_exists(meta.file_path)) {
        fragment_tbl_ptr = FragmentTable::from_file(meta.file_path.c_str());
    } else {
        fragment_tbl_ptr = FragmentTable::create_ptr(
            meta.file_path.c_str(), 
            meta.head_capacity, 
            meta.tail_capacity,
            meta.jump_capacity
        );
    }
    return fragment_tbl_ptr;
}

FragmentTable* FragmentTable::from_file(const char *file_path) throw (invalid_argument, runtime_error) {
    int fd = open(file_path, O_RDWR);
    if (-1 == fd) {
        _handle_runtime_error("Can NOT open the region file");
    }

    // Gonna mmap the entire file, we need to know the file size first.
    struct stat sb;
    if (-1 == fstat(fd, &sb)) {
        _handle_runtime_error("Can NOT get file stat");
    }
    if (sb.st_size < sizeof (FragmentTable)) {
        // at least
        _handle_runtime_error("Invalid region file. It's too small.");
    }
    size_t map_size = sb.st_size;

    off_t offset = 0;
    off_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
    size_t map_offset = map_size + offset - pa_offset;
    int map_prot = PROT_READ | PROT_WRITE;
    int map_flags = MAP_SHARED | MAP_NONBLOCK | MAP_POPULATE;
    uint8_t *map_addr = (uint8_t *) mmap(NULL, map_offset, map_prot, map_flags, fd, pa_offset);

    if (MAP_FAILED == map_addr) {
        _handle_runtime_error("Can NOT mmap FragmentTable.");
    }

    FragmentTable *fragment_tbl_ptr = FragmentTable::from_ptr(map_addr);
    fragment_tbl_ptr->fd = fd;
    fragment_tbl_ptr->map_offset = map_offset;

    return fragment_tbl_ptr;
}

void FragmentTable::_handle_runtime_error(const char* error) throw (runtime_error) {
    stringstream ss;
    ss << error << "\tError message: " << errno;
    throw runtime_error(ss.str());
}

void FragmentTable::_update_tail_last(uint32_t tail_next) {
    uint32_t expected;
    bool exchanged = false;

    // DO care about the order
    do {
        expected = tail_last;
        exchanged = tail_last.compare_exchange_strong(expected, tail_next);
    } while (!exchanged);

    // force tail_last to relocate to the current one    
    this->_get_tail_entry(expected)->set_next(tail_next);
    // increase `tail_avail`
    this->_update_tail_avail(true);
}

vector<JmpDistance> FragmentTable::get_jumps(FragmentEntry *object_entry_ptr) throw (invalid_argument) {
    if (object_entry_ptr == nullptr || !object_entry_ptr->is_object_entry()) {
        throw invalid_argument("Invalid object entry pointer");
    }
    vector<JmpDistance> jumps;
    if (object_entry_ptr->object.has_no_jumps()) {
        return jumps;
    }
    uint32_t jump_head = object_entry_ptr->object.jump_head;
    JmpTable* jt_ptr = this->get_jump_table();
    JmpEntry* je_ptr = nullptr;
    uint32_t jmps_per_entry = sizeof (JmpEntryData)/sizeof(JmpDistance);
    do {
        if (jump_head == NULL_INDEX) {
            break;
        }
        je_ptr = jt_ptr->get_entry(jump_head);
        for (uint32_t index = 0; index < jmps_per_entry; index++) {
            if (je_ptr->data[index] == 0) {
                break;
            }
            jumps.push_back(je_ptr->data[index]);
        }
        jump_head = je_ptr->next;
    } while (true);

    return jumps;
}

bool FragmentTable::free_entry(uint64_t key){
    vector<uint32_t> fragment_indices;
    return this->free_entry(key, fragment_indices);
}

bool FragmentTable::free_entry(uint64_t key, vector<uint32_t>& fragment_indices) {
    const uint32_t head_index = static_cast<uint32_t> (key % head_capacity);
    FragmentEntry *entry = this->_get_head_entry(head_index), *prev_entry = nullptr;
    uint32_t tail_next = 0;
    uint16_t tag28 = utils::get_tag_28_bits(key);

    if (entry->is_free(false)) {
        if (entry->is_tail()) {
            // no such entry
            return false;
        }
    }

    do {
        if (entry->get_tag() == tag28) {
            break;
        }
        tail_next = entry->get_next();
        prev_entry = entry;
        entry = this->_get_tail_entry(tail_next);
    } while (entry != nullptr && !entry->is_tail());

    if (entry == nullptr) {
        return false; // no such key
    }
    
    if(entry->is_object_entry()){
        fragment_indices = this->get_fragment_indices(entry);        
    }
    
    // now we'll release jump entries
    if (entry->is_object_entry()) {
        if (!entry->object.has_no_jumps()) {
            uint32_t jump_head = entry->object.jump_head;
            JmpTable* jt_ptr = this->get_jump_table();
            jt_ptr->free_entries(jump_head);
        }
    }

    entry->free();

    if (tail_next == 0) {
        // the head entry is free but its `next` still point to a valid tail entry
        return true; // no tail to recover
    }
    // and push back the tail
    prev_entry->set_next(entry->get_next());
    entry->set_next(NULL_INDEX);
    this->_update_tail_last(tail_next); 

    return true;
}

vector<uint32_t> FragmentTable::get_fragment_indices(FragmentEntry *object_entry_ptr){
    vector<uint32_t> fragment_indices;    
    if (object_entry_ptr == nullptr || !object_entry_ptr->is_object_entry()) {
        return fragment_indices;
    }
    
    fragment_indices.push_back(object_entry_ptr->object.region_head);
    
    if (object_entry_ptr->object.has_no_jumps()) {
        return fragment_indices;
    }        
    vector<JmpDistance> jumps = this->get_jumps(object_entry_ptr);

    uint32_t index = 0;
    for(auto jmp: jumps){
        fragment_indices.push_back(
            fragment_indices[index++] + jmp
        );
    }
    return fragment_indices;
}

vector<uint32_t> FragmentTable::get_fragment_indices(uint64_t object_key) throw (invalid_argument){
    FragmentEntry* entry_ptr = this->get_entry(object_key);
    return this->get_fragment_indices(entry_ptr);
}

void FragmentTable::_validate_capacities(uint32_t &head_capacity, uint32_t &tail_capacity, uint32_t &jump_capacity) throw (invalid_argument) {
    head_capacity = (head_capacity == 0) ? MAX_INT20 : head_capacity;
    tail_capacity = (tail_capacity == 0) ? MAX_INT16 : tail_capacity;
    jump_capacity = (jump_capacity == 0) ? MAX_INT20 : jump_capacity;

    if (head_capacity > MAX_INT20) {
        throw invalid_argument("Head capacity is too large");
    }
    if (tail_capacity > MAX_INT16) {
        throw invalid_argument("Tail capacity is too large");
    }
    if (jump_capacity > MAX_INT20) {
        throw invalid_argument("Jump capacity is too large");
    }
}

FragmentTable *FragmentTable::from_ptr(uint8_t *buffer) throw (invalid_argument) {
    if (buffer == nullptr) {
        throw invalid_argument("Invalid buffer");
    }
    FragmentTable *ft_ptr = (FragmentTable *) buffer;
    if (ft_ptr->version != FT_VERSION) {
        throw invalid_argument("Invalid version");
    }
    return ft_ptr;
}

FragmentTable *FragmentTable::init_ptr(uint8_t *buffer, uint32_t head_capacity, uint32_t tail_capacity, uint32_t jump_capacity) throw (invalid_argument) {
    FragmentTable::_validate_capacities(head_capacity, tail_capacity, jump_capacity);

    FragmentTable *ft_ptr = (FragmentTable *) buffer;

    // initialize header
    ft_ptr->version = FT_VERSION;
    ft_ptr->header_size = static_cast<uint16_t> (sizeof (FragmentTable));
    ft_ptr->head_capacity = head_capacity;
    ft_ptr->tail_capacity = tail_capacity;
    ft_ptr->map_offset = 0;

    // initialize head & tail
    ft_ptr->clear();

    // initialize JumpTable
    uint8_t *jmp_ptr = ft_ptr->get_jump_section();
    JmpTable::init_ptr(jmp_ptr, jump_capacity);

    return ft_ptr;
}

FragmentTable *FragmentTable::create_ptr(uint32_t head_capacity, uint32_t tail_capacity, uint32_t jump_capacity) throw (invalid_argument) {
    FragmentTable::_validate_capacities(head_capacity, tail_capacity, jump_capacity);
    uint32_t size = FragmentTable::calculate_size(head_capacity, tail_capacity, jump_capacity);
    uint8_t *buffer = new uint8_t[size]();
    return FragmentTable::init_ptr(buffer, head_capacity, tail_capacity, jump_capacity);
}

FragmentTable * FragmentTable::create_ptr(const char *file_path, uint32_t head_capacity, uint32_t tail_capacity, uint32_t jump_capacity) throw (invalid_argument) {
    FragmentTable * fragment_table_ptr = FragmentTable::create_ptr(head_capacity, tail_capacity, jump_capacity);

    int fd = open(file_path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (-1 == fd) {
        _handle_runtime_error(
            utils::str_format("Can NOT open the FragmentTable file at %", file_path).c_str()
        );
    }

    uint8_t* buffer = (uint8_t*) fragment_table_ptr;
    off_t offset = 0;
    size_t count = fragment_table_ptr->get_size();

    if (-1 == pwrite(fd, buffer, count, offset)) {
        _handle_runtime_error("Can NOT write FragmentTable data.");
    }
    // close the file
    if (0 != close(fd)) {
        _handle_runtime_error("Can NOT close files");
    }
    FragmentTable::release_ptr(fragment_table_ptr);

    return FragmentTable::from_file(file_path);
}

uint32_t FragmentTable::calculate_size(uint32_t head_capacity, uint32_t tail_capacity, uint32_t jump_capacity) throw (invalid_argument) {
    FragmentTable::_validate_capacities(head_capacity, tail_capacity, jump_capacity);
    uint32_t size = sizeof (FragmentTable);
    size += (head_capacity + tail_capacity) * sizeof (FragmentEntry);
    size += JmpTable::calculate_size(jump_capacity);
    return size;
}

uint32_t FragmentTable::_reserve_tail_entry() throw (runtime_error) {
    bool exchanged = false;
    uint32_t expected, tail_first_new;
    if (tail_avail == 0) {
        throw runtime_error("No available free tail");
    }

    // forward tail_first
    do {
        expected = tail_first;
        tail_first_new = this->_get_tail_entry(expected)->next;
        exchanged = tail_first.compare_exchange_strong(expected, tail_first_new);
    } while (!exchanged);

    // decrease tail_avail by 1
    this->_update_tail_avail(false);

    return expected;
}

FragmentEntry * FragmentTable::_get_head_entry(uint32_t head_index) throw (invalid_argument) {
    if (head_index >= this->head_capacity) {
        throw invalid_argument("Index out of range");
    }
    FragmentEntry *entry = (FragmentEntry *)this->get_head_section();
    return entry + head_index;
}

FragmentEntry * FragmentTable::_get_tail_entry(uint32_t tail_index) throw (invalid_argument) {
    if (tail_index >= this->tail_capacity) {
        throw invalid_argument("Index out of range");
    }
    if (tail_index == 0) {
        return nullptr;
    }
    FragmentEntry *entry = (FragmentEntry *)this->get_tail_section();
    return entry + tail_index;
}

void FragmentTable::release_ptr(FragmentTable *ptr) {
    if(ptr->map_offset == 0){
        delete[](uint8_t *) ptr;
        return;
    }
    int32_t fd = ptr->fd;
    uint8_t *map_addr = (uint8_t *) ptr;
    if (-1 == munmap(map_addr, ptr->map_offset)) {
        _handle_runtime_error("Can NOT unmap FragmentTable files");
    }

    if (-1 == close(fd)) {
        _handle_runtime_error("Can NOT close FragmentTable files");
    }    
}

uint8_t *FragmentTable::get_ptr(uint32_t offset) {
    return (uint8_t *)this +offset;
}

uint8_t *FragmentTable::get_head_section() {
    uint32_t offset = sizeof (FragmentTable);
    return this->get_ptr(offset);
}

uint8_t *FragmentTable::get_tail_section() {
    uint32_t offset = sizeof (FragmentTable);
    offset += this->head_capacity * sizeof (FragmentEntry);
    return this->get_ptr(offset);
}

uint8_t *FragmentTable::get_jump_section() {
    uint32_t offset = sizeof (FragmentTable);
    offset += (this->head_capacity + this->tail_capacity) * sizeof (FragmentEntry);
    return this->get_ptr(offset);
}

JmpTable *FragmentTable::get_jump_table() {
    return JmpTable::from_ptr(this->get_jump_section());
}

uint32_t FragmentTable::get_size() {
    uint32_t size = sizeof (FragmentTable);
    size += (head_capacity + tail_capacity) * sizeof (FragmentEntry);
    size += this->get_jump_table()->get_size();
    return size;
}

void FragmentTable::clear() {
    // mark all entries are free
    FragmentEntry *entry_ptr = (FragmentEntry *)this->get_head_section();
    uint32_t times = this->head_capacity + this->tail_capacity;
    while (times > 0) {
        times--;
        entry_ptr->free();
        entry_ptr += 1;
    }

    // entry now points to the last one of Tail
    this->tail_avail = this->tail_capacity - 1;
    this->tail_first = 1; // 0 is used as NULL_INDEX
    this->tail_last = this->tail_capacity - 1;

    // chain entries in tail
    entry_ptr -= 1;
    entry_ptr->set_next(NULL_INDEX);

    times = this->tail_avail;
    while (times > 1) {
        entry_ptr -= 1;
        entry_ptr->set_next(times);
        times -= 1;
    }

}

void FragmentTable::_update_tail_avail(bool increase) throw (runtime_error) {
    uint32_t expected, desired;
    bool exchanged;

    do {
        expected = tail_avail;
        desired = increase ? expected + 1 : expected - 1;
        if (desired >= this->tail_capacity) {
            throw runtime_error("Tails overflow.");
        }
        exchanged = tail_avail.compare_exchange_strong(expected, desired);
    } while (!exchanged);
}

FragmentEntry *FragmentTable::_acquire_entry(uint64_t key) throw (runtime_error) {
    const uint32_t head_index = static_cast<uint32_t> (key % head_capacity);
    FragmentEntry *entry = this->_get_head_entry(head_index);
    uint32_t reserved_tail;
    uint16_t tag28 = utils::get_tag_28_bits(key);

    if (!entry->is_free()) {
        if (entry->get_tag() == tag28) {
            throw runtime_error("Hash collision is found.");
        }
        do {
            // loop to the end of the chain
            // and append the new entry
            while (!entry->is_tail()) {
                if (entry->get_tag() == tag28) {
                    throw runtime_error("Hash collision is found.");
                }
                entry = this->_get_tail_entry(entry->next);
            }

            reserved_tail = this->_reserve_tail_entry();
            // we're aware of concurrent modifications
            // so here's the loop
        } while (!entry->set_next(reserved_tail));

        entry = this->_get_tail_entry(reserved_tail);
    }

    return entry;
}

void FragmentTable::update_region_head(uint64_t object_key, uint32_t region_head) throw (invalid_argument, runtime_error){
    FragmentEntry * entry_ptr = this->get_entry(object_key);
    if (entry_ptr == nullptr) {
        throw invalid_argument("No such object key");
    }
    if(!entry_ptr->is_object_entry()){
        throw invalid_argument("Must be a key associated with an object");
    }
    entry_ptr->object.region_head = region_head;
}

uint32_t FragmentTable::get_fragment_index(uint64_t object_key, uint32_t jump_index) throw (invalid_argument, runtime_error){
    FragmentEntry * entry_ptr = this->get_entry(object_key);
    if (entry_ptr == nullptr) {
        throw invalid_argument("No such object key");
    }
    if(!entry_ptr->is_object_entry()){
        throw invalid_argument("Must be a key associated with an object");
    }
    uint32_t jmp_head = entry_ptr->object.jump_head;
    if(jump_index == 0) {
        return jmp_head;
    }

    uint32_t fragment_index = entry_ptr->object.region_head, data_index;
    JmpTable* jmp_tbl_ptr = this->get_jump_table();
    JmpEntry* jmp_entry_ptr = nullptr;
    uint32_t jmps_per_entries = sizeof(JmpEntryData) / sizeof(JmpDistance);

    do {
        jmp_entry_ptr = jmp_tbl_ptr->get_entry(jmp_head);
        if (jmp_entry_ptr == nullptr) {
            throw runtime_error("Invalid jump head");
        }

        data_index = 0;
        JmpDistance* data = jmp_entry_ptr->data;
        while(jump_index > 0){
            jump_index -= 1;
            fragment_index += data[data_index];
            if(++data_index >= jmps_per_entries){
                break;
            }
        }

        jmp_head = jmp_entry_ptr->next;
    } while (jump_index != 0);   

    return fragment_index; 
}

void FragmentTable::update_jump_entry(uint64_t object_key, uint32_t jump_index, JmpDistance jump_value) throw (invalid_argument, runtime_error) {
    FragmentEntry * entry_ptr = this->get_entry(object_key);
    if (entry_ptr == nullptr) {
        throw invalid_argument("No such object key");
    }
    if(!entry_ptr->is_object_entry()){
        throw invalid_argument("Must be a key associated with an object");
    }

    uint32_t jmp_head = entry_ptr->object.jump_head;
    JmpTable* jmp_tbl_ptr = this->get_jump_table();
    JmpEntry* jmp_entry_ptr = nullptr;
    uint32_t jmp_data_size = sizeof(JmpEntryData);
    uint32_t index = jump_index;

    do {
        jmp_entry_ptr = jmp_tbl_ptr->get_entry(jmp_head);
        if (jmp_entry_ptr == nullptr) {
            throw runtime_error("Invalid jump head");
        }
        jmp_head = jmp_entry_ptr->next;
        if(index > jmp_data_size){
            index -= jmp_data_size;
            continue;
        }
        break;
    } while (true);
    
    jmp_entry_ptr->data[index] = jump_value;
}

FragmentEntry *FragmentTable::get_entry(uint64_t key) {
    const uint32_t head_index = static_cast<uint32_t> (key % head_capacity);
    FragmentEntry *entry = this->_get_head_entry(head_index);
    uint16_t tag28 = utils::get_tag_28_bits(key);

    if (entry->is_free(false)) {
        if (entry->is_tail()) {
            // no such entry
            return nullptr;
        }
        // may be down the chain
        entry = this->_get_tail_entry(entry->next);
    }

    do {
        if (entry->get_tag() == tag28) {
            return entry;
        }
        entry = this->_get_tail_entry(entry->next);
    } while (entry != nullptr);

    return nullptr;
}

FragmentEntry *FragmentTable::get_object_entry(uint64_t key) {
    FragmentEntry *entry = this->get_entry(key);
    if (entry == nullptr || entry->is_object_entry()) {
        return entry;
    }
    uint64_t object_key = entry->get_object_key();
    return this->get_entry(object_key);
}

FragmentEntry *FragmentTable::reserve_entry(uint64_t object_key, uint16_t fragment_type, uint32_t region_head, vector<JmpDistance> jmps) throw (invalid_argument, runtime_error) {
    if (fragment_type > 0xF) {
        throw invalid_argument("Invalid fragment type. Must be in the range of [0, 0xF]");
    }
    FragmentEntry *entry = this->_acquire_entry(object_key);
    uint16_t tag28 = utils::get_tag_28_bits(object_key);

    entry->set_tag(tag28);
    entry->set_type(fragment_type);
    entry->set_next(NULL_INDEX);
    entry->object.region_head = region_head;

    if (jmps.size() == 0) { // no jump entries to allocate
        return entry;
    }

    // store jump data
    uint32_t jmp_data_size = sizeof (JmpEntryData);
    uint16_t jumps_per_entry = sizeof(JmpEntryData) / sizeof(JmpDistance);
    uint16_t entries_count = (jmps.size()  / jumps_per_entry) + 1;

    JmpTable *jt_ptr = this->get_jump_table();
    uint32_t jump_head = jt_ptr->reserve_entries(entries_count);

    entry->object.jump_head = jump_head;

    for (uint32_t jmp_index = 0; jmp_index < jmps.size();) {
        JmpEntry *entry_ptr = jt_ptr->get_entry(jump_head);
        memset(entry_ptr->data, 0, jmp_data_size);
        for (uint32_t data_index = 0;  (data_index < jumps_per_entry) && (jmp_index < jmps.size());) {
            entry_ptr->data[data_index++] = jmps[jmp_index++];
        }

        // next JmpEntry
        jump_head = entry_ptr->next;
    }

    return entry;
}

FragmentEntry * FragmentTable::reserve_entry(uint64_t name_key, uint64_t object_key) throw (invalid_argument, runtime_error) {
    FragmentEntry *entry = this->_acquire_entry(name_key);
    uint16_t fragment_type = 0xF; // type for name entries
    uint16_t tag28 = utils::get_tag_28_bits(name_key);

    entry->set_tag(tag28);
    entry->set_type(fragment_type);
    entry->set_next(NULL_INDEX);

    entry->name.object_key[1] = static_cast<uint32_t> (object_key);
    entry->name.object_key[0] = static_cast<uint32_t> (object_key >> 32);

    return entry;
}
#endif /* FRAGMENTTABLE_H */
