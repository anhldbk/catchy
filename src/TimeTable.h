/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * TimeTable represents a LRU list of fragments in a region.
 * File:   TimeTable.h
 * Author: anhld2
 *
 * Created on June 30, 2017, 9:41 AM
 */

#ifndef TIMETABLE_H
#define TIMETABLE_H

#include <stdint.h>
#include <atomic>
#include <exception>
#include <iostream>
#include "logging/inc.h"
using namespace std;

#define TIME_TABLE_VERSION 1
#define MAX_INT20 1048576

struct TimeEntry{
    uint64_t key;
};

// A circular structure of pre-allocated entries
// Schema: <header><entries>
struct TimeTable{
    // currently version 1
    uint32_t version : 16;
    // size of header in bytes (currently 12 bytes)
    uint32_t header_size : 16;    

    uint32_t entry_capacity;    
    atomic<uint32_t> first;
    atomic<uint32_t> last;

    /**
     * Create a new instance of TimeTable.
     * All related data areas will be initialized as well.
     * @param entry_capacity Maximum entries can be stored. 0 means MAX_INT20
     * @return Pointer to the newly created instance
     */
    static TimeTable * create_ptr(uint32_t entry_capacity = 0) throw (invalid_argument);

    /**
     * Create a new TimeTable pointer form a TimeTable-serialized data
     * @return The pointer
     */
    static TimeTable *from_ptr(uint8_t *buffer) throw (invalid_argument);

    /**
     * Release pointers created by `create_ptr` or `from_ptr`
     * @param ptr   Pointer to an instance of Region
     */
    static void release_ptr(TimeTable* ptr) throw (invalid_argument, runtime_error);


    /**
     * Initialize a memory buffer with specific capacities.
     * Note: The buffer must be allocated with enough memory. You can use `calculate_size()` 
     *      to know how many bytes the buffer should use.
     * @param buffer The buffer
     * @param entry_capacity Maximum entries can be stored. 0 means MAX_INT20
     * @return Pointer to an initialized TimeTable instance (having the same address as `buffer`)
     */
    static TimeTable * init_ptr(uint8_t *buffer, uint32_t entry_capacity = 0) throw (invalid_argument);

    static void _validate_capacities(uint32_t &entry_capacity) throw (invalid_argument);    

    TimeEntry& reserve_entry() throw(runtime_error);

    TimeEntry& get_entry(uint32_t index) throw(invalid_argument);
    
    /**
     * Free the first entry
     * @return The freed entry     
     */    
    TimeEntry free_entry() throw(runtime_error);;

    /**
     * Write data into an entry
     * @param index The reserved index 
     * @param event The event
     * @return Nothing. If the index is invalid, an exception of `invalid_argument` is thrown.
     */
    void write_entry(uint32_t index, TimeEntry event) throw (invalid_argument);

    void write_entry(TimeEntry event) throw(runtime_error);
    
    void write_entry(uint64_t key) throw(runtime_error);    

    /**
     * Calculate the overall size of a TimeTable
     * @param entry_capacity Number of available event entries. 0 means MAX_INT24
     * @return The size in bytes
     */
    static uint32_t calculate_size(uint32_t entry_capacity) throw (invalid_argument);    

    uint32_t get_size();

    bool is_full();
};

void TimeTable::write_entry(TimeEntry event) throw(runtime_error){
    this->reserve_entry() = event;
}

void TimeTable::write_entry(uint64_t key) throw(runtime_error){
    this->reserve_entry().key = key;
}

void TimeTable::release_ptr(TimeTable* ptr) throw (invalid_argument, runtime_error){
    if(ptr == nullptr){
        throw invalid_argument("Invalid pointer");
    }
    delete[] ptr;
}

bool TimeTable::is_full(){
    return ((this->last + 1) % this->entry_capacity) == this->first; 
}

TimeTable* TimeTable::create_ptr(uint32_t entry_capacity) throw (invalid_argument){
    const uint32_t size = TimeTable::calculate_size(entry_capacity);
    uint8_t* buffer = new uint8_t[size];
    return TimeTable::init_ptr(buffer, entry_capacity);
}

TimeTable* TimeTable::from_ptr(uint8_t *buffer) throw (invalid_argument){
    if(buffer == nullptr){
        throw invalid_argument("Invalid buffer");
    }
    TimeTable* ptr = (TimeTable*)buffer;
    if(ptr->version != TIME_TABLE_VERSION){
        throw invalid_argument("Invalid buffer");
    }
    return ptr;
}

TimeTable* TimeTable::init_ptr(uint8_t *buffer, uint32_t entry_capacity) throw (invalid_argument){
    if(buffer == nullptr){
        throw invalid_argument("Invalid buffer");
    }
    TimeTable* ptr = (TimeTable*)buffer;
    ptr->version = TIME_TABLE_VERSION;
    ptr->header_size = sizeof(TimeTable);
    ptr->entry_capacity = entry_capacity;
    ptr->first = 0;
    ptr->last = 0;
    return ptr;
}

void TimeTable::_validate_capacities(uint32_t &entry_capacity) throw (invalid_argument){
    auto logger = Logger::get_instance("TimeTable");
    if(entry_capacity == 0){
        logger->info("No capacity is specified. It will be set to MAX_INT20 by default.");
        entry_capacity = MAX_INT20;
        return;
    }
    if(entry_capacity >MAX_INT20){
        logger->warn("Entries capacity is greater than the predefined threshold");
        logger->warn("It will be capped to MAX_INT20");
        entry_capacity = MAX_INT20;
    }
}

TimeEntry& TimeTable::reserve_entry() throw(runtime_error){
    if(this->is_full()){
        throw runtime_error("TimeTable is full");
    }
    
    uint32_t expected, desired;
    
    do{
        expected = this->last;
        desired = (expected + 1) % this->entry_capacity;
    } while(!this->last.compare_exchange_strong(expected, desired));

    return this->get_entry(expected);
}

TimeEntry& TimeTable::get_entry(uint32_t index) throw(invalid_argument){
    if(index > this->entry_capacity){
        throw invalid_argument("Invalid index");
    }
    TimeEntry* entries_ptr = (TimeEntry*)(this + sizeof(TimeTable));
    return entries_ptr[index];
}

TimeEntry TimeTable::free_entry() throw(runtime_error){
    uint32_t expected, desired;
    TimeEntry entry;
    do{
        expected = this->first;
        desired = (expected + 1) % this->entry_capacity;
        if(desired > this->last){
            throw runtime_error("Trying to free an empty table");
        }
        entry = this->get_entry(expected);
    } while(!this->first.compare_exchange_strong(expected, desired));
    return entry;
}

void TimeTable::write_entry(uint32_t index, TimeEntry event) throw (invalid_argument){
    this->get_entry(index) = event;
}

uint32_t TimeTable::calculate_size(uint32_t entry_capacity) throw (invalid_argument){
    TimeTable::_validate_capacities(entry_capacity);
    return sizeof(TimeTable) + sizeof(TimeEntry)* entry_capacity;
}

uint32_t TimeTable::get_size(){
    return sizeof(TimeTable) + sizeof(TimeEntry)* this->entry_capacity;
}

#endif /* TIMETABLE_H */

