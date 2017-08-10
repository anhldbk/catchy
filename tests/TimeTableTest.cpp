#include <gtest/gtest.h>
#include <atomic>
#include "src/TimeTable.h"
#include <stdint.h>
#include <iostream>
#include <string.h>
#include "src/utils.h"

using namespace std;

#define MAGIC_KEY   0x5555

TEST(TimeTableTest, testCreatePtr) {
    uint32_t entry_capacity = 256;

    try {
        TimeTable* table_ptr = TimeTable::create_ptr(entry_capacity);
        EXPECT_NE(table_ptr, nullptr);
        TimeTable::release_ptr(table_ptr);
    } 
    catch (...) {
        FAIL() << "Exception";
    }
}

TEST(TimeTableTest, testCreatePtrOverflow) {
    uint32_t entry_capacity = MAX_INT20 + 1;

    try {
        TimeTable* table_ptr = TimeTable::create_ptr(entry_capacity);
        FAIL() << "Must throw exception of `invalid_argument`";
    } 
    catch(invalid_argument e){

    }
    catch (...) {
        FAIL() << "Must throw exception of `invalid_argument`";
    }
}

TEST(TimeTableTest, testReserveEntry) {
    uint32_t entry_capacity = 256;

    TimeTable* table_ptr = TimeTable::create_ptr(entry_capacity);
    EXPECT_NE(table_ptr, nullptr);
    EXPECT_EQ(table_ptr->first, 0);

    TimeEntry& entry = table_ptr->reserve_entry();
    EXPECT_EQ(table_ptr->last, 1);

    uint64_t magic = 0x5555;
    entry.key = magic;

    EXPECT_EQ(table_ptr->get_entry(0).key, magic);

    TimeTable::release_ptr(table_ptr);
}

TEST(TimeTableTest, testFree) {
    uint32_t entry_capacity = 256;

    TimeTable* table_ptr = TimeTable::create_ptr(entry_capacity);
    EXPECT_NE(table_ptr, nullptr);
    EXPECT_EQ(table_ptr->first, 0);

    TimeEntry& entry_ref = table_ptr->reserve_entry();
    entry_ref.key = MAGIC_KEY;
    EXPECT_EQ(table_ptr->last, 1);

    table_ptr->reserve_entry();
    EXPECT_EQ(table_ptr->last, 2);

    TimeEntry entry = table_ptr->free_entry();
    EXPECT_EQ(table_ptr->first, 1);
    EXPECT_EQ(table_ptr->last, 2);    
    EXPECT_EQ(entry.key, MAGIC_KEY); // must free the oldest one

    table_ptr->free_entry();
    EXPECT_EQ(table_ptr->first, 2);
    EXPECT_EQ(table_ptr->last, 2); 
    
    try {
        table_ptr->free_entry();
        FAIL() << "Must throw exception of `runtime_error`";
    } 
    catch(runtime_error e){

    }
    catch (...) {
        FAIL() << "Must throw exception of `runtime_error`";
    }

    TimeTable::release_ptr(table_ptr);
}

TEST(TimeTableTest, testFull) {
    uint32_t entry_capacity = 3;

    TimeTable* table_ptr = TimeTable::create_ptr(entry_capacity);
    EXPECT_NE(table_ptr, nullptr);
    EXPECT_EQ(table_ptr->first, 0);

    table_ptr->reserve_entry();
    EXPECT_EQ(table_ptr->last, 1);    

    table_ptr->reserve_entry();
    EXPECT_EQ(table_ptr->last, 2);    
    
    try {
        table_ptr->reserve_entry();
        FAIL() << "Must throw exception of `runtime_error`";
    } 
    catch(runtime_error e){

    }
    catch (...) {
        FAIL() << "Must throw exception of `runtime_error`";
    }
    EXPECT_EQ(table_ptr->last, 2); 

    table_ptr->free_entry();
    EXPECT_EQ(table_ptr->first, 1);

    try {
        table_ptr->reserve_entry();
    } 
    catch (...) {
        FAIL() << "Must success";
    }
    EXPECT_EQ(table_ptr->last, 0); 

    TimeTable::release_ptr(table_ptr);    
}

TEST(TimeTableTest, testReserveConcurrent) {
    uint32_t entry_capacity = 1024;

    TimeTable* table_ptr = TimeTable::create_ptr(entry_capacity);
    EXPECT_NE(table_ptr, nullptr);
    EXPECT_EQ(table_ptr->first, 0);

    uint32_t loop = 10, threads = 5;    
    auto callback = [&](){
        for (uint32_t i = 0; i < loop; i++) {
            table_ptr->reserve_entry();
        }
    };
    utils::run_in_pool(callback, threads);
    EXPECT_EQ(table_ptr->last, loop * threads);    

    TimeTable::release_ptr(table_ptr);
}

TEST(TimeTableTest, testReserveFreeConcurrent) {
    uint32_t entry_capacity = 100;

    TimeTable* table_ptr = TimeTable::create_ptr(entry_capacity);
    EXPECT_NE(table_ptr, nullptr);
    EXPECT_EQ(table_ptr->first, 0);

    uint32_t loop = 20, threads = 6;    
    auto callback = [&](){
        bool success = false;
        for (uint32_t i = 0; i < loop; i++) {
            success = false;
            do{
                try{
                    table_ptr->reserve_entry();
                    success = true;
                } catch(runtime_error e){
                    table_ptr->free_entry();
                }
            } while(!success);
        }
    };
    utils::run_in_pool(callback, threads);
    EXPECT_EQ(table_ptr->is_full(), true);    

    TimeTable::release_ptr(table_ptr);
}