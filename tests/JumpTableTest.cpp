/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <gtest/gtest.h>
#include <thread>
#include <poll.h>
#include <iostream>
#include <bitset>
#include <stdexcept>
#include "src/JumpTable.h"
#include "src/utils.h"
using namespace std;

typedef JumpTable<uint32_t> JumpTableInt;

TEST(JumpTableTest, testCreatePtr) {
    uint32_t entry_nums = 32;
    JumpTableInt* ptr = JumpTableInt::create_ptr(entry_nums);
    // check if the first block is free
    EXPECT_EQ(ptr->is_free_entry(0), true);
    JumpTableInt::release_ptr(ptr);
}

TEST(JumpTableTest, testReserveBlocks) {
    uint32_t entry_nums = 10;
    JumpTableInt* ptr = JumpTableInt::create_ptr(entry_nums);

    // 0(th) block is not used
    EXPECT_EQ(ptr->reserve_entries(2), 1);
    EXPECT_EQ(ptr->reserve_entries(7), 3);

    try {
        ptr->reserve_entries(10);
        FAIL() << "Must have an exception";
    } catch (runtime_error const& err) {
        SUCCEED();
    } catch (...) {
        FAIL() << "Must have an exception";
    }
    JumpTableInt::release_ptr(ptr);
}

TEST(JumpTableTest, testReserveBlocksChained) {
    uint32_t entry_nums = 10;
    JumpTableInt* ptr = JumpTableInt::create_ptr(entry_nums);

    // 0(th) block is not used
    EXPECT_EQ(ptr->reserve_entries(7), 1);
    for (uint32_t i = 1; i < 7; i++) {
        EXPECT_EQ(ptr->get_entry(i)->next, i + 1);
    }
    EXPECT_EQ(ptr->get_entry(7)->next, 0);

    JumpTableInt::release_ptr(ptr);
}

TEST(JumpTableTest, testReserveBlocksConcurrent) {
    uint32_t entry_nums = 300;
    JumpTableInt* ptr = JumpTableInt::create_ptr(entry_nums);

    const int max_loop = 10, max_thread = 4, count = 5;
    auto modify = [&]() {
        for (int i = 0; i < max_loop; i++) {
            ptr->reserve_entries(count);
            poll(NULL, 0, 1);
        }
    };
    utils::run_in_pool(modify, max_thread);

    // must add one coz we omit the first block
    const uint32_t entry_index = count * max_loop * max_thread + 1;
    EXPECT_EQ(ptr->reserve_entries(count), entry_index);

    JumpTableInt::release_ptr(ptr);
}

TEST(JumpTableTest, testFreeBlocks) {
    uint32_t entry_nums = 30, index;
    JumpTableInt* ptr = JumpTableInt::create_ptr(entry_nums);

    // 0(th) tail entry is not used
    index = ptr->reserve_entries(10);
    EXPECT_EQ(index, 1);

    index = ptr->reserve_entries(10);
    EXPECT_EQ(index, 11);

    ptr->free_entries(11);
    index = ptr->reserve_entries(10);
    EXPECT_EQ(index, 11);

    JumpTableInt::release_ptr(ptr);
}
