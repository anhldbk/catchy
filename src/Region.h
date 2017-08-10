/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Region.h
 * Author: anhld2
 *
 * Created on June 19, 2017, 11:20 AM
 */

#ifndef REGION_H
#define REGION_H
#include "Fragment.h"
#include "TimeTable.h"
#include "FragmentTable.h"
#include "common.h"
#include "utils.h"
#include <atomic>
#include <errno.h>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include "logging/inc.h"
using namespace std;

#define MAX_INT24 16777216
#define REGION_VERSION 1

// Region = <header><time-table><fragments>

struct Region {
    // header
    uint32_t version : 16; // currently version 1
    uint32_t header_size : 16; // size of header in byte

    // each type is associated with a different fragment size
    uint32_t fragment_type : 4;
    // number of fragments
    uint32_t fragment_capacity : 24;
    // extra field
    uint32_t extra : 4;

    // maximum size (in bytes) of data can be stored in fragments.
    uint32_t fragment_data_capacity;
    uint32_t fragment_size;

    // Information about mmaped area
    // Descriptor of the associated region file
    int32_t fd;
    size_t map_offset;

    // index of the last free offset (to speed up looking process)
    atomic<uint32_t> last_free_fragment;

    // number of available fragments
    atomic<uint32_t> fragment_avail;

    // fragment[i] is free to use if bit i(th) of state is set
    atomic<uint8_t> state[(MAX_INT24 >> 3) + 1]; // ~ 2mb

    /**
     * Get a Region pointer from meta information
     * If the region does NOT exist, it will be created and initialized.
     * If it does exist, it will be loaded instead.
     * @param meta  Meta information about a region
     * @return A Region pointer.
     */
    static Region* load(const RegionMeta& meta) throw (invalid_argument, runtime_error);

    /**
     * Create a new instance of Region.
     * NOTE: This method will keep only the Region's header in memory.
     *
     * @param  file_path   A valid path of a wrtiable file.          * @param
     * fragment_type Fragment type, must fall into the range of [1, 12]
     * @param  fragment_capacity    Maximum number of fragments can be used in
     * this region
     * @return Pointer to an initialized Region instance
     */
    static Region *create_ptr(const char *file_path, uint8_t fragment_type, uint32_t fragment_capacity) throw (invalid_argument, runtime_error);

    /**
     * Create a new Region pointer form a Region-serialized file
     * @param  file_path   A valid path of a readable file.
     * @return The pointer
     */
    static Region *from_file(const char *file_path) throw (invalid_argument, runtime_error);

    /**
     * Get a Region pointer from a Region-serialized data
     * @return The pointer
     */
    static Region *from_ptr(uint8_t *buffer) throw (invalid_argument);

    static void _validate_capacities(uint8_t &fragment_type, uint32_t &fragment_capacity) throw (invalid_argument);

    void _update_fragment_avail(bool increase = true, uint32_t delta = 1) throw (runtime_error);

    /**
     * Release pointers created by `create_ptr` or `from_file`
     * @param ptr   Pointer to an instance of Region
     */
    static void release_ptr(Region *ptr) throw (invalid_argument, runtime_error);

    /**
     * Write a fragment into this region. The fragment index will be determined
     * automatically
     *  at runtime.
     * @param  fragment_ptr Pointer to a fragment
     * @return If success, returns the index. Otherwise, throws exceptions.
     */
    uint32_t _write_fragment(Fragment *fragment_ptr) throw (invalid_argument, runtime_error);

    /**
     * Write a fragment into a location in this region
     * @param  reserved_index Index of the reserved fragment
     * @param  fragment_ptr Pointer to a fragment
     * @return If success, returns the fragment's index. Otherwise, throws
     * exceptions.
     */
    uint32_t write_fragment(uint32_t reserved_index, Fragment *fragment_ptr) throw (invalid_argument, runtime_error);

    /**
     * Read a fragment by its index
     * @param  fragment_index The index
     * @param  data Pointer to the memory area where read data will be stored
     * @param  size Size of the area
     * @return If success, returns the fragment's pointer. Otherwise, returns
     * null.
     */
    Fragment *read_fragment(int32_t fragment_index, uint8_t *data, uint32_t size) throw (invalid_argument);

    /**
     * Read a fragment by its index
     * @param  fragment_index The index
     * @param  fragment_ptr Pointer to a Fragment
     * @return If success, returns the fragment's pointer. Otherwise, returns
     * null.
     */
    Fragment *read_fragment(int32_t fragment_index, Fragment *fragment_ptr) throw (invalid_argument);

    /**
     * Free a fragment by clearing the associated state bit.
     * @param fragment_index Index of the fragment to delete.
     * @param update_fragment_avail Set this parameter to `false` if you don't
     * want
     *      to update `fragment_avail` in the end. It may be useful when you call
     *      with `free_fragments` where fragments are freed in batch.
     * @return Nothing. If there's no fragment with such index, throws exceptions.
     */
    void _free_fragments(uint32_t fragment_index, bool update_fragment_avail = true) throw (invalid_argument);

    /**
     * Reserve fragments (in batch)
     * @param  key Name key or content key. The key will be added into the
     * TimeTable ahead.
     * @param  fragments_num Number of fragments to reserve
     * @return Indices of reserved fragments.
     *      If there's no fragments with such quantity, throw exceptions of
     * `runtime_error`
     * @note    If there's a runtime error, you may need to kick out the oldest
     * fragments
     */
    vector<uint32_t> reserve_fragments(uint64_t key, uint32_t fragments_num) throw (runtime_error);


    /**
     * Get relative jumps from jump indices
     * @param jmp_indices  Jump indices
     * @return Vector containing relative jumps
     */
    vector<JmpDistance> get_jumps(vector<uint32_t> jmp_indices) throw (std::runtime_error);
    /**
     * Free reserved fragments (in batch). Note that: the associated key in the
     * TimeTable
     *      is not freed on the fly. It's an intentional design. Fragments can be
     * only freed
     *      if the associated Region is full. You should ask the time table to
     * kick the oldest entry
     *      (which contains the key), then acquires fragment indices via JumpTable
     * before invoking
     *      this method.
     * @param  fragments_num Number of fragments to reserve
     * @return Indices of reserved fragments.
     *      If there's no fragments with such quantity, throw exceptions of
     * `runtime_error`
     */
    void free_fragments(vector<uint32_t> fragment_indices) throw (invalid_argument);

    /**
     * Free the oldest entry in the TimeTable
     * @return Key of the entry. If the table is empty, an exception of
     * `runtime_error` is thrown.
     */
    uint64_t free_oldest() throw (runtime_error);

    /**
     * Calculate the overall size of regions.
     * NOTE: Regions may be too large to load/mmap into memory.
     * @param fragment_type Fragment type, must fall into the range of [1, 12]
     * @param fragment_capacity Number of entries in Tail. 0 means MAX_INT20
     * @return The size in bytes
     */
    static uint32_t calculate_size(uint8_t fragment_type, uint32_t fragment_capacity = 0) throw (invalid_argument);

    /**
     * Regions can NOT be fully loaded into mem. Only region header & time table
     * are `mmapped` instead
     * This method calculates how many bytes a region header occupies in RAM.
     * NOTE: Fragments are 512-bytes aligned. And so the headers must be a
     * multiple of 512.
     * @return The size in bytes
     */
    static uint32_t calculate_mem_size(uint32_t fragment_capacity) throw (invalid_argument);

    uint32_t get_mem_size();
    uint32_t get_size();
    /**
     * Check if this region is full (no more available fragment)
     * @return Returns true if the region is full. Otherwise, returns false.
     */
    bool is_full();

    TimeTable *get_time_table();

    /**
     * Acquire a free fragment.
     * @param update_fragment_avail Set this parameter to `false` if you don't
     * want
     *      to update `fragment_avail` in the end. It may be useful when you call
     *      with `reserve_fragments` where fragments are acquired in batch.
     * @return Index of the acquired fragment
     */
    uint32_t _acquire_free_fragment(bool update_fragment_avail = true) throw ( runtime_error);

    bool _is_free_fragment(uint32_t fragment_index);

    static void _handle_runtime_error(const char *error) throw (runtime_error);

    void _update_last_free_fragment(uint32_t index);
};

vector<JmpDistance> Region::get_jumps(vector<uint32_t> jmp_indices) throw(runtime_error){
    vector<JmpDistance> res;
    uint32_t length = jmp_indices.size(), delta;
    if(length < 2){
        return res;
    }

    for(uint32_t index = 1; index < length; index++){
        if(jmp_indices[index] > jmp_indices[index-1]){
            delta = jmp_indices[index] - jmp_indices[index-1];
        } else {
            delta = this->fragment_capacity - (jmp_indices[index-1]- jmp_indices[index]);
        }
        if(delta > 0xFF){
            throw runtime_error("Maximum jump distance is reached");
        }
        res.push_back(delta);
    }

    return res;
}

Region* Region::load(const RegionMeta& meta) throw (invalid_argument, runtime_error) {
    Region* region_ptr = nullptr;
    if (utils::file_exists(meta.file_path)) {
        region_ptr = Region::from_file(meta.file_path.c_str());
    } else {
        region_ptr = Region::create_ptr(meta.file_path.c_str(), meta.fragment_type, meta.fragment_capacity);
    }
    return region_ptr;
}


uint64_t Region::free_oldest() throw (runtime_error) {
    return this->get_time_table()->free_entry().key;
}

bool Region::is_full() {
    return this->fragment_avail == 0;
}

TimeTable *Region::get_time_table() {
    return TimeTable::from_ptr((uint8_t *)this +sizeof (Region));
}

uint32_t Region::write_fragment(uint32_t reserved_index, Fragment *fragment_ptr) throw (invalid_argument, runtime_error) {
    if (reserved_index > this->fragment_capacity) {
        throw invalid_argument("Invalid index");
    }
    if (fragment_ptr == nullptr) {
        throw invalid_argument("Invalid Fragment pointer");
    }

    // now write the fragment into disk
    uint8_t *buffer = (uint8_t *) fragment_ptr;
    uint32_t size = this->fragment_size;
    uint32_t offset = reserved_index * size + this->get_mem_size();
    if (-1 == pwrite(this->fd, buffer, size, offset)) {
        _handle_runtime_error("Can NOT write the fragment.");
    }

    return reserved_index;
}

vector<uint32_t> Region::reserve_fragments(uint64_t key, uint32_t fragments_num) throw (runtime_error) {
    // decrease the number of available fragments first
    this->_update_fragment_avail(false, fragments_num);

    vector<uint32_t> reserved;
    for (uint32_t index = 0; index < fragments_num; index++) {
        reserved.push_back(this->_acquire_free_fragment(false));
    }

    this->get_time_table()->write_entry(key);

    return reserved;
}

void Region::free_fragments(vector<uint32_t> fragment_indices) throw ( invalid_argument) {
    for (uint32_t fragment_index : fragment_indices) {
        this->_free_fragments(fragment_index, false);
    }
    this->_update_fragment_avail(true, fragment_indices.size());
}

void Region::_free_fragments(uint32_t fragment_index, bool update_fragment_avail) throw (invalid_argument) {
    atomic<uint8_t> *bucket_ptr = &this->state[fragment_index >> 3];
    uint8_t pos = fragment_index % 8;
    uint8_t expected, desired;
    bool exchanged, is_free;

    // update state
    do {
        expected = bucket_ptr->load();
        is_free = !((expected >> pos) & 1);
        if (is_free) {
            break;
        }
        desired &= ~(1 << pos);
        exchanged = bucket_ptr->compare_exchange_strong(expected, desired);
    } while (!exchanged);

    this->_update_last_free_fragment(fragment_index);

    if (update_fragment_avail) {
        // only update the number of available fragments at the end
        this->_update_fragment_avail();
    }
}

void Region::_update_last_free_fragment(uint32_t index) {
    uint32_t expected;
    bool exchanged = false;

    // modify this->last_free_fragment
    do {
        expected = this->last_free_fragment;
        exchanged =
                this->last_free_fragment.compare_exchange_strong(expected, index);
    } while (!exchanged);
}

void Region::_update_fragment_avail(bool increase, uint32_t delta) throw (runtime_error) {
    uint32_t expected, desired;
    do {
        expected = this->fragment_avail;
        desired = increase ? expected + delta : expected - delta;
        if (!increase && (desired > expected)) {
            this->_handle_runtime_error("No more available fragments");
        }
        if (increase && (desired < expected)) {
            this->_handle_runtime_error("Fragments overflow");
        }
        if (desired > this->fragment_capacity) {
            this->_handle_runtime_error("Fragments overflow");
        }
    } while (!this->fragment_avail.compare_exchange_strong(expected, desired));
}

bool Region::_is_free_fragment(uint32_t fragment_index) {
    atomic<uint8_t> *bucket_ptr = &this->state[fragment_index >> 3];
    return ((bucket_ptr->load() >> (fragment_index % 8)) & 1) == 0;
}

uint32_t Region::_acquire_free_fragment(bool update_fragment_avail) throw ( runtime_error) {
    if (update_fragment_avail) {
        this->_update_fragment_avail(false);
    }
    // try to acquire a free fragment
    atomic<uint8_t> *bucket_ptr;
    uint8_t expected, desired, pos;
    uint32_t count = 0, index = this->last_free_fragment;
    bool is_free, exchanged;

    auto valid = [&]() {
        index = (index > this->fragment_capacity) ? 0 : (index + 1);
        return ++count < this->fragment_capacity;
    };

    do {
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

        break;

    } while (valid());

    return index;
}

void Region::_handle_runtime_error(const char *error) throw (runtime_error) {
    stringstream ss;
    ss << error << "\tError code: " << errno;
    throw runtime_error(ss.str());
}

uint32_t
Region::calculate_size(uint8_t fragment_type,
        uint32_t fragment_capacity) throw (invalid_argument) {
    Region::_validate_capacities(fragment_type, fragment_capacity);

    uint32_t size = Region::calculate_mem_size(fragment_capacity);

    uint32_t fragment_size = Fragment::calculate_size(fragment_type);
    size += fragment_size * fragment_capacity;

    return size;
}

uint32_t
Region::calculate_mem_size(uint32_t fragment_capacity) throw (invalid_argument) {
    uint32_t size = sizeof (Region);
    size += TimeTable::calculate_size(fragment_capacity);
    // ensure `size` is a multiple of 512
    size = (size == 0) ? 0 : (((size - 1) >> 9) + 1) << 9;
    return size;
}

uint32_t Region::get_mem_size() {
    return Region::calculate_mem_size(this->fragment_capacity);
}

uint32_t Region::get_size() {
    return Region::calculate_size(this->fragment_type, this->fragment_capacity);
}

Region *Region::from_ptr(uint8_t *buffer) throw (invalid_argument) {
    if (buffer == nullptr) {
        throw invalid_argument("Invalid buffer");
    }

    Region *region_ptr = (Region *) buffer;
    if (region_ptr->version != REGION_VERSION) {
        throw invalid_argument("Invalid version");
    }
    return region_ptr;
}

void Region::_validate_capacities(uint8_t &fragment_type, uint32_t &fragment_capacity) throw (invalid_argument) {
    if (fragment_type < 1 || fragment_type > 12) {
        throw invalid_argument(
                "Invalid fragment type. Must fall into the range of [1, 12]");
    }

    fragment_capacity = (fragment_capacity == 0) ? MAX_INT24 : fragment_capacity;
    if (fragment_capacity > MAX_INT24) {
        throw invalid_argument(
                "Fragment capacity is too large. Maximum value is MAX_INT24.");
    }
}

void Region::release_ptr(Region *ptr) throw (invalid_argument, runtime_error) {
    if (ptr == nullptr) {
        throw invalid_argument("Invalid pointer");
    }
    int32_t fd = ptr->fd;
    uint8_t *map_addr = (uint8_t *) ptr;
    if (-1 == munmap(map_addr, ptr->map_offset)) {
        _handle_runtime_error("Can NOT unmap region headers");
    }

    if (-1 == close(fd)) {
        _handle_runtime_error("Can NOT close region files");
    }
}

Region *Region::create_ptr(const char *file_path, uint8_t fragment_type, uint32_t fragment_capacity) throw (invalid_argument, runtime_error) {
    Region::_validate_capacities(fragment_type, fragment_capacity);

    int fd = open(file_path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (-1 == fd) {
        _handle_runtime_error("Can NOT open the Region file");
    }

    size_t region_size = Region::calculate_size(fragment_type, fragment_capacity);
    if (-1 == ftruncate(fd, region_size)) {
        _handle_runtime_error("Can NOT truncate the region file.");
    }

    // prepare an empty fragment to write
    Fragment *fragment_ptr = Fragment::create_ptr(fragment_type);

    // initialize the header
    off_t offset = 0;
    size_t count = Region::calculate_mem_size(fragment_capacity);
    uint8_t *buffer = new uint8_t[count]();
    Region *region_ptr = (Region *) buffer;
    region_ptr->version = REGION_VERSION;
    region_ptr->header_size = static_cast<uint16_t> (sizeof (Region));
    region_ptr->fragment_type = fragment_type;
    region_ptr->fragment_capacity = fragment_capacity;
    region_ptr->fd = fd;
    region_ptr->last_free_fragment = 0;
    region_ptr->fragment_avail = fragment_capacity;
    region_ptr->fragment_data_capacity = fragment_ptr->get_data_capacity();
    region_ptr->fragment_size = fragment_ptr->get_size();
    const uint32_t loop = (MAX_INT24 >> 3) + 1;
    for (uint32_t i = 0; i < loop; i++) {
        region_ptr->state[i] = 0;
    }

    TimeTable::init_ptr(buffer + sizeof (Region), fragment_capacity);

    // now write the header & fragments into disk
    if (-1 == pwrite(fd, buffer, count, offset)) {
        _handle_runtime_error("Can NOT write the header.");
    }
    offset += count;

    count = fragment_ptr->get_size();
    for (uint32_t i = 0; i < fragment_capacity; i++) {
        if (-1 == pwrite(fd, fragment_ptr, count, offset)) {
            _handle_runtime_error("Can NOT write fragments.");
        }
        offset += count;
    }

    // close the file
    if (0 != close(fd)) {
        _handle_runtime_error("Can NOT close files");
    }

    // clean up
    Fragment::release_ptr(fragment_ptr);
    delete[] buffer;

    return Region::from_file(file_path);
}

Region *Region::from_file(const char *file_path) throw (invalid_argument, runtime_error) {
    int fd = open(file_path, O_RDWR);
    if (-1 == fd) {
        _handle_runtime_error("Can NOT open the region file");
    }

    // TODO: Is there any way to properly map (without remap) ?
    // we don't know `fragment_capacity` ahead
    size_t map_size = sizeof (Region);

    // To obtain file size
    struct stat sb;
    if (-1 == fstat(fd, &sb)) {
        _handle_runtime_error("Can NOT get file stat");
    }
    if (sb.st_size < map_size) {
        _handle_runtime_error("Invalid region file. It's too small.");
    }

    // We're gonna `mmap` only region meta = region header + time table
    off_t offset = 0;
    off_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
    size_t map_offset = map_size + offset - pa_offset;
    int map_prot = PROT_READ | PROT_WRITE;
    int map_flags = MAP_SHARED | MAP_NONBLOCK | MAP_POPULATE;
    uint8_t *map_addr = (uint8_t *) mmap(NULL, map_offset, map_prot, map_flags, fd, pa_offset);

    if (MAP_FAILED == map_addr) {
        _handle_runtime_error("Can NOT mmap region meta.");
    }

    Region *region_ptr = Region::from_ptr(map_addr);

    // we need to remap in account of `fragment_capacity`
    size_t new_map_size = Region::calculate_mem_size(region_ptr->fragment_capacity);
    map_addr = (uint8_t *) mremap(map_addr, map_size, new_map_size, MREMAP_MAYMOVE);

    if (MAP_FAILED == map_addr) {
        _handle_runtime_error("Can NOT remap region meta.");
    }

    region_ptr = Region::from_ptr(map_addr);
    TimeTable::from_ptr((uint8_t *) region_ptr + sizeof (Region)); // test if we can properly load
    region_ptr->fd = fd;
    region_ptr->map_offset = map_offset;

    return region_ptr;
}

uint32_t Region::_write_fragment(Fragment *fragment_ptr) throw (invalid_argument, runtime_error) {
    if (fragment_ptr == nullptr) {
        throw invalid_argument("Invalid data");
    }

    // TODO: using CAS,acquire a free fragment and write data into it.
    if (0 == this->fragment_avail) {
        _handle_runtime_error("Out of free fragments");
    }

    // try to acquire a free fragment
    uint32_t fragment_index = this->_acquire_free_fragment();
    return this->write_fragment(fragment_index, fragment_ptr);
}

Fragment *Region::read_fragment(int32_t fragment_index, Fragment *fragment_ptr) throw (invalid_argument) {
    if (fragment_ptr == nullptr) {
        throw invalid_argument("Invalid Fragment pointer");
    }
    uint8_t *data = (uint8_t *) fragment_ptr;
    uint32_t size = fragment_ptr->get_size();
    return this->read_fragment(fragment_index, data, size);
}

Fragment *Region::read_fragment(int32_t fragment_index, uint8_t *data, uint32_t size) throw (invalid_argument) {
    if (fragment_index > this->fragment_capacity) {
        throw invalid_argument("Invalid fragment index");
    }
    if (this->fragment_size > size) {
        throw invalid_argument("Data buffer is too small");
    }
    if (data == nullptr) {
        throw invalid_argument("Invalid data buffer");
    }
    uint32_t offset = fragment_index * this->fragment_size + this->get_mem_size();

    if (-1 == pread(this->fd, data, size, offset)) {
        _handle_runtime_error("Can NOT write the fragment.");
    }
    return Fragment::from_ptr(data);
}

#endif /* REGION_H */
