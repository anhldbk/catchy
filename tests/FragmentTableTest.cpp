/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <gtest/gtest.h>
#include <thread>
#include <poll.h>
#include <vector>
#include <iostream>
#include <bitset>
#include <stdexcept>
#include "src/FragmentTable.h"
#include "src/utils.h"
using namespace std;

#define MAX_HEADS 1048576
#define MAX_TAILS 65536
#define MAX_JUMPS 262144

TEST(FragmentTableTest, testCreatePtr) {
    uint32_t head_cap = 3, tail_cap = 30, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);

    EXPECT_EQ(ptr->head_capacity, head_cap);
    EXPECT_EQ(ptr->tail_capacity, tail_cap);
    EXPECT_EQ(ptr->tail_avail, tail_cap - 1);

    // check if tail entries are properly chained
    FragmentEntry* entry = (FragmentEntry*) ptr->get_tail_section();
    entry += 1; // 0th entry is omitted

    for (uint32_t index = 1; index < tail_cap - 1; index++) {
        EXPECT_EQ(entry->next, index + 1);
        entry += 1;
    }
    // and the final entry
    EXPECT_EQ(entry->next, 0);

    JmpTable* jt_ptr = ptr->get_jump_table();
    EXPECT_EQ(jt_ptr->entry_capacity, jmp_cap);
    EXPECT_EQ(jt_ptr->entry_avail, jmp_cap - 1);
    EXPECT_EQ(jt_ptr->last_free_entry, 1);

    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testCreateAndStoreToFile) {
    uint32_t head_cap = 3, tail_cap = 30, jmp_cap = 20;
    string cwd = utils::get_current_dir();
    string file_path = utils::str_format("%/fragment_tables.bin", cwd);
    FragmentTable* ptr = FragmentTable::create_ptr(file_path.c_str(), head_cap, tail_cap, jmp_cap);

    uint16_t fragment_type = 0xA;
    uint64_t object_key = 23;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    FragmentEntry* entry_ptr = ptr->reserve_entry(object_key, fragment_type, region_head, jmps);

    auto verify_entry = [&](){
        EXPECT_EQ(entry_ptr->get_type(), fragment_type);
        EXPECT_EQ(entry_ptr->get_tag(), utils::get_tag_28_bits(object_key));
        EXPECT_EQ(entry_ptr->get_next(), 0);
        
        // Test for jumping entries
        vector<JmpDistance> jmps_get = ptr->get_jumps(entry_ptr);
        EXPECT_EQ(jmps_get.size(), jmps.size());        
        for(uint8_t index = 0; index < jmps.size(); index++){
            EXPECT_EQ(jmps_get[index], jmps[index]);                    
        }
    };
    verify_entry();    

    EXPECT_EQ(entry_ptr, ptr->get_entry(object_key));    
    FragmentTable::release_ptr(ptr);

    //  reload to check
    ptr = FragmentTable::from_file(file_path.c_str());
    entry_ptr = ptr->get_entry(object_key);
    verify_entry();
    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testReserveTailEntry) {
    uint32_t head_cap = 3, tail_cap = 100, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);

    uint32_t loop = 10, threads = 5;
    auto callback = [&]() {
        for (uint32_t i = 0; i < loop; i++) {
            ptr->_reserve_tail_entry();
        }
    };
    utils::run_in_pool(callback, threads);

    EXPECT_EQ(ptr->tail_avail, tail_cap - 1 - loop * threads);

    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testReserveEntry) {
    uint32_t head_cap = 3, tail_cap = 100, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);

    uint16_t fragment_type = 0x1F;
    uint64_t object_key = 23;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    try {
        ptr->reserve_entry(object_key, fragment_type, region_head, jmps);
        FAIL() << "Must have an exception";
    } catch (invalid_argument const& err) {
    } catch (...) {
        FAIL() << "Must have an exception";
    }

    fragment_type = 0xA;
    FragmentEntry* entry_ptr = ptr->reserve_entry(object_key, fragment_type, region_head, jmps);

    auto verify_entry = [&](){
        EXPECT_EQ(entry_ptr->get_type(), fragment_type);
        EXPECT_EQ(entry_ptr->get_tag(), utils::get_tag_28_bits(object_key));
        EXPECT_EQ(entry_ptr->get_next(), 0);
        
        // Test for jumping entries
        vector<JmpDistance> jmps_get = ptr->get_jumps(entry_ptr);
        EXPECT_EQ(jmps_get.size(), jmps.size());        
        for(uint8_t index = 0; index < jmps.size(); index++){
            EXPECT_EQ(jmps_get[index], jmps[index]);                    
        }
    };

    verify_entry();

    // try to add another duplicated object key
    try {
        ptr->reserve_entry(object_key, fragment_type, region_head, jmps);
        FAIL() << "Must have an exception";
    } catch (runtime_error const& err) {
    } catch (...) {
        FAIL() << "Must have an exception";
    }    

    // try with another object key that may share the same head with the previous one
    object_key += head_cap;
    entry_ptr = ptr->reserve_entry(object_key, fragment_type, region_head, jmps);    
    verify_entry();

    // map a name key to the object key
    uint64_t name_key = 0x232;
    entry_ptr = ptr->reserve_entry(name_key, object_key);
    // ensure the entry is now a name entry
    EXPECT_EQ(entry_ptr->is_object_entry(), false);

    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testReserveEntry2) {
    uint32_t head_cap = 3, tail_cap = 100, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);

    uint16_t fragment_type = 0x0A;
    uint64_t object_key = 23;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    FragmentEntry* entry_ptr = ptr->reserve_entry(object_key, fragment_type, region_head, jmps);

    auto verify_entry = [&](){
        EXPECT_EQ(entry_ptr->get_type(), fragment_type);
        EXPECT_EQ(entry_ptr->get_tag(), utils::get_tag_28_bits(object_key));
        EXPECT_EQ(entry_ptr->get_next(), 0);
        
        // Test for jumping entries
        vector<JmpDistance> jmps_get = ptr->get_jumps(entry_ptr);
        EXPECT_EQ(jmps_get.size(), jmps.size());        
        for(uint8_t index = 0; index < jmps.size(); index++){
            EXPECT_EQ(jmps_get[index], jmps[index]);                    
        }
    };

    verify_entry();
    jmps[0] = 3;
    
    try{
        ptr->update_jump_entry(object_key, 0, 3);
    } catch(...){
        FAIL() << "Must not have any exception";
    }
    entry_ptr = ptr->get_entry(object_key);
    verify_entry();
    
    region_head = 100;
    ptr->update_region_head(object_key, region_head);
    entry_ptr = ptr->get_entry(object_key);
    EXPECT_EQ(entry_ptr->object.region_head, region_head);

    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testGetFragmentIndex) {
    uint32_t head_cap = 3, tail_cap = 100, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);

    uint16_t fragment_type = 0x0A;
    uint64_t object_key = 23;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    FragmentEntry* entry_ptr = ptr->reserve_entry(object_key, fragment_type, region_head, jmps);

    EXPECT_EQ(ptr->get_fragment_index(object_key, 0), region_head);
    uint32_t length = jmps.size();
    for(uint32_t index = 0; index < length; index++){
        region_head += jmps[index];
        EXPECT_EQ(ptr->get_fragment_index(object_key, index+1), region_head);
    }
    // uint32_t length = jmps.size();

    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testGetEntry) {
    uint32_t head_cap = 3, tail_cap = 100, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);

    uint16_t fragment_type = 0x0A;
    uint64_t object_key = 23;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    uint64_t name_key = 0x232;
    
    ptr->reserve_entry(name_key, object_key);
    ptr->reserve_entry(object_key, fragment_type, region_head, jmps);

    // check for non-existing keys
    EXPECT_EQ(ptr->get_entry(200), nullptr);
    EXPECT_EQ(ptr->get_object_entry(200), nullptr);

    FragmentEntry* entry_ptr = ptr->get_entry(object_key);
    FragmentEntry* entry_ptr_by_object_key = ptr->get_object_entry(object_key);
    FragmentEntry* entry_ptr_by_name_key = ptr->get_object_entry(name_key);

    EXPECT_EQ(entry_ptr, entry_ptr_by_name_key);
    EXPECT_EQ(entry_ptr_by_object_key, entry_ptr_by_name_key);

    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testReserveFreeEntry) {
    uint32_t head_cap = 3, tail_cap = 100, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);
    JmpTable* jt_ptr = ptr->get_jump_table();
    uint16_t fragment_type = 0x0A;
    uint64_t object_key = 23, name_key = 32;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    ptr->reserve_entry(object_key, fragment_type, region_head, jmps);
    ptr->reserve_entry(name_key, object_key);
    ptr->free_entry(object_key);
    ptr->free_entry(name_key);    

    EXPECT_EQ(ptr->tail_avail, ptr->tail_capacity - 1);
    EXPECT_EQ(jt_ptr->entry_avail, jt_ptr->entry_capacity - 1);
    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testReserveFreeEntry2) {
    uint32_t head_cap = 10, tail_cap = 100, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);
    uint16_t fragment_type = 0x0A;
    uint64_t object_key = 23;
    uint32_t region_head = 1;
    uint32_t delta = 0;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    // try to reserve multiple keys that may sharing the same head
    // and so, belong to the same chain
    for(uint32_t index = 0; index < 4; index ++){
        ptr->reserve_entry(object_key+ delta, fragment_type, region_head, jmps);
        delta += head_cap;
    }

    // we can still free the head without affecting tail entries
    ptr->free_entry(object_key);
    EXPECT_EQ(ptr->get_entry(object_key), nullptr);

    // the selected key is in the middle of the chain
    uint64_t prev_selected_key = object_key + head_cap;
    uint64_t selected_key = object_key + 2* head_cap;
    uint64_t post_selected_key = object_key + 3* head_cap;
    ptr->free_entry(selected_key);

    EXPECT_EQ(ptr->tail_avail, ptr->tail_capacity - 1 - 2);
    
    // check if `prev` and `post` are chained directly
    FragmentEntry* prev_entry_ptr = ptr->get_entry(prev_selected_key);
    FragmentEntry* post_entry_ptr = ptr->get_entry(post_selected_key);    
    FragmentEntry* prev_entry_next_ptr = ptr->_get_tail_entry(prev_entry_ptr->next);
    
    EXPECT_EQ(prev_entry_next_ptr, post_entry_ptr);
    EXPECT_EQ(post_entry_ptr->is_tail(), true);

    ptr->free_entry(prev_selected_key);
    post_entry_ptr = ptr->get_entry(post_selected_key);        
    EXPECT_EQ(post_entry_ptr != nullptr, true);    
    EXPECT_EQ(ptr->tail_avail, ptr->tail_capacity - 1 - 1);

    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testReserveFreeEntry3) {
    uint32_t head_cap = 3, tail_cap = 100, jmp_cap = 20;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);
    JmpTable* jt_ptr = ptr->get_jump_table();

    uint16_t fragment_type = 0x0A;
    uint64_t object_key = 23, name_key = 32;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    ptr->reserve_entry(object_key, fragment_type, region_head, jmps);
    
    vector<uint32_t> fragment_indices = ptr->get_fragment_indices(object_key);
    EXPECT_EQ(fragment_indices.size(), jmps.size() + 1);    
    EXPECT_EQ(fragment_indices[0], region_head);       

    for(uint32_t index = 1; index < fragment_indices.size(); index++){
        region_head += jmps[index-1];
        EXPECT_EQ(fragment_indices[index], region_head);               
    }

    EXPECT_EQ(ptr->tail_avail, ptr->tail_capacity - 1);
    uint32_t jumps_per_entry = sizeof(JmpEntryData) / sizeof(JmpDistance);
    uint32_t jump_entries = (jmps.size() / jumps_per_entry) + 1;
    // Jump Table always has 0th entry reserved
    EXPECT_EQ(jt_ptr->entry_avail, jt_ptr->entry_capacity - jump_entries - 1);
    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testReserveEntryConcurrent) {
    uint32_t head_cap = 100, tail_cap = 100, jmp_cap = 100;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);

    uint16_t fragment_type = 0x0A;
    uint64_t object_key = 23;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    uint32_t max_loop = 3, max_thread = 5;
    auto callback = [&](uint64_t key){
        for(uint32_t index = 0; index < max_loop; index++){
            ptr->reserve_entry(key, fragment_type, region_head, jmps);            
            key += head_cap;
        }
    };
    vector<thread> threads;
    for(uint32_t index = 0; index < max_thread; index++){
        threads.push_back(thread(callback, index));
    }
    // wait for all of them to finish
    for (auto& th : threads) {
        th.join();        
    }

    EXPECT_EQ(ptr->tail_avail, ptr->tail_capacity - 1 - (max_loop -1 )* max_thread);
    FragmentTable::release_ptr(ptr);
}

TEST(FragmentTableTest, testReserveAndFreeEntryConcurrent) {
    uint32_t head_cap = 100, tail_cap = 100, jmp_cap = 100;
    FragmentTable* ptr = FragmentTable::create_ptr(head_cap, tail_cap, jmp_cap);

    uint16_t fragment_type = 0x0A;
    uint64_t object_key = 23;
    uint32_t region_head = 1;
    vector<JmpDistance> jmps = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

    uint32_t max_loop = 3, max_thread = 5;
    auto callback = [&](uint64_t key){
        vector<uint64_t> keys;
        for(uint32_t index = 0; index < max_loop; index++){
            ptr->reserve_entry(key, fragment_type, region_head, jmps);            
            keys.push_back(key);
            key += head_cap;
        }
        for(auto k: keys){
            ptr->free_entry(k);
        }
    };
    vector<thread> threads;
    for(uint32_t index = 0; index < max_thread; index++){
        threads.push_back(thread(callback, index));
    }
    // wait for all of them to finish
    for (auto& th : threads) {
        th.join();        
    }

    EXPECT_EQ(ptr->tail_avail, ptr->tail_capacity - 1);
    FragmentTable::release_ptr(ptr);
}
