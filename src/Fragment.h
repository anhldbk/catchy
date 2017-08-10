/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Fragment.h
 * Author: anhld2
 *
 * Created on June 23, 2017, 9:02 AM
 */

#ifndef FRAGMENT_H
#define FRAGMENT_H

#include <stdint.h>
#include <stdexcept>
#include <string.h>
#include <atomic>
using namespace std;

#define FRAGMENT_VERSION 1

//////////////////////////////////////////////////////
// schema of Fragment: <header> <body>
// Fragment size must be a multiple of 512. That means
// sizeOf(header) + sizeOf(body) = 512*k
// Object partial data is hosted in body
// 

struct Fragment {
    // header (8 bytes in size)

    // currently version 1
    uint32_t version : 16;
    // size of header in bytes (currently 8 bytes)
    uint32_t header_size : 16;

    uint32_t type : 4;
    // size of the data stored in the body (in byte)
    // in bytes (maximum 2mb)
    uint32_t data_size : 21;

    uint32_t extra : 7;

    /**
     * Get the size of the current fragment
     * @return The fragment size in bytes
     */
    uint32_t get_size();

    /**
     * Get the size of data stored in this fragment
     * @return The data size in bytes
     */
    uint32_t get_data_size();

    /**
     * Get the maximum size of data can be stored in this fragment
     * @return The data size in bytes
     */
    uint32_t get_data_capacity();

    /**
     * Get the pointer to the stored data
     * @return The pointer
     */
    uint8_t* get_data();

    void clear();

    /**
     * Set fragment data
     * @param buffer Pointer to data buffer
     * @param size Number of bytes to set
     * @return Nothing. Throws exceptions if `size` is larger than `data capacity`.
     */
    void set_data(uint8_t* buffer, uint32_t size) throw(invalid_argument);
    
    void set_data(string data) throw(invalid_argument);

    /**
     * Create a new instance of Fragment.
     * All related data areas will be initialized as well.
     * @param type Fragment's type. Must fall into the range of [1, 12].
     *      fragment_size = 2^(9 + i) to align to disk sectors (512 = 2^9bytes)
     * @return Pointer to the newly created instance
     */
    static Fragment* create_ptr(uint32_t type) throw (invalid_argument);

    /**
     * Initialize a memory buffer with specific capacities.
     * Note: The buffer must be allocated with enough memory. You can use `calculate_size()` 
     *      to know how many bytes the buffer should use.
     * @param buffer The buffer     
     * @param type Fragment's type. Must fall into the range of [1, 12].
     * @return Pointer to an initialized FragmentTable instance (having the same address as `buffer`)
     */
    static Fragment* init_ptr(uint8_t *buffer, uint32_t type) throw (invalid_argument);

    /**
     * Create a new Fragment pointer form a Fragment-serialized data
     * @return The pointer
     */
    static Fragment *from_ptr(uint8_t *buffer) throw (invalid_argument);

    /**
     * Release pointers created by `create_ptr`
     * @param ptr   Pointer to an instance of Fragment
     */
    static void release_ptr(Fragment *ptr);

    /**
     * Calculate how many bytes this structure should occupy for a specific fragment type
     * @param type Fragment's type. Must fall into the range of [1, 12].
     * @return The size in bytes
     */
    static uint32_t calculate_size(uint32_t type) throw (invalid_argument);

    static uint32_t calculate_data_capacity(uint32_t type) throw (invalid_argument);
    
    static void _validate_capacities(uint32_t type) throw (invalid_argument);
};

void Fragment::clear(){
    this->data_size = 0;    
    memset(this->get_data(), 0, this->get_data_size());    
}

void Fragment::set_data(string data) throw(invalid_argument){
    this->set_data((uint8_t*)data.c_str(), data.length());
}

void Fragment::set_data(uint8_t* buffer, uint32_t size) throw(invalid_argument){
    if(size == 0){
        return this->clear();
    }
    if(size > this->get_data_capacity()){
        throw invalid_argument("Data size is too large");
    }
    if(buffer == nullptr){
        throw invalid_argument("Invalid data buffer");
    }
    this->data_size = size;
    memcpy(this->get_data(), buffer, size);
}

uint32_t Fragment::get_size() {
    return Fragment::calculate_size(this->type);
}

uint32_t Fragment::get_data_size() {
    return this->data_size;
}

uint32_t Fragment::get_data_capacity() {
    return this->get_size() - sizeof (Fragment);
}

uint8_t* Fragment::get_data() {
    uint32_t offset = sizeof (Fragment);
    return (uint8_t *)this +offset;
}

Fragment* Fragment::create_ptr(uint32_t type) throw (invalid_argument) {
    Fragment::_validate_capacities(type);
    uint32_t size = Fragment::calculate_size(type);
    uint8_t *buffer = new uint8_t[size](); // TODO: use jemalloc or some kind of pools
    return Fragment::init_ptr(buffer, type);
}

Fragment* Fragment::init_ptr(uint8_t *buffer, uint32_t type) throw (invalid_argument) {
    Fragment* ptr = (Fragment*) buffer;
    // initialize header
    ptr->version = FRAGMENT_VERSION;
    ptr->header_size = sizeof (Fragment);
    ptr->type = type;
    ptr->data_size = 0;

    return ptr;
}

Fragment* Fragment::from_ptr(uint8_t *buffer) throw (invalid_argument) {
    if (buffer == nullptr) {
        throw invalid_argument("Invalid buffer");
    }

    Fragment* ptr = (Fragment*) buffer;
    if (ptr->version != FRAGMENT_VERSION) {
        throw invalid_argument("Invalid version");
    }

    Fragment::_validate_capacities(ptr->type);
    return ptr;
}

void Fragment::release_ptr(Fragment *ptr) {
    delete[](uint8_t *) ptr;
}

uint32_t Fragment::calculate_size(uint32_t type) throw (invalid_argument) {
    Fragment::_validate_capacities(type);
    return 1 << (9 + type);
}

uint32_t Fragment::calculate_data_capacity(uint32_t type) throw (invalid_argument) {
    return Fragment::calculate_size(type) - sizeof (Fragment);
}

void Fragment::_validate_capacities(uint32_t type) throw (invalid_argument) {
    if (type < 1 || type > 12) {
        throw invalid_argument("Invalid type. Must fall into the range of [1, 12]");
    }
}

#endif /* FRAGMENT_H */

