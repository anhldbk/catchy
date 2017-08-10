/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <gtest/gtest.h>
#include <stdint.h>
#include <poll.h>
#include <vector>
#include <string.h>
#include <iostream>
#include <stdexcept>
#include "src/FragmentEntry.h"
#include "src/utils.h"
using namespace std;

TEST(FragmentEntryTest, testCreate) {
    FragmentEntry entry;
    EXPECT_EQ(entry.is_free(), true);
}

TEST(FragmentEntryTest, testConcurrentAcquire) {
    const uint32_t size = sizeof (FragmentEntry);
    uint8_t * buffer = new uint8_t[size];
    memset(buffer, 0, size);
    FragmentEntry* entry = (FragmentEntry*) buffer;
    uint8_t acquire_count = 0;

    auto callback = [&]() {
        if (entry->is_free()) {
            acquire_count += 1;
        }
    };

    // run in multiple threads
    utils::run_in_pool(callback, 10);
    // and only one can acquire the entry
    EXPECT_EQ(acquire_count, 1);
    delete[] buffer;
}

TEST(FragmentEntryTest, testFree){
    FragmentEntry entry;
    EXPECT_EQ(entry.is_free(), true);   
    EXPECT_EQ(entry.is_free(false), false);           
    entry.free();
    EXPECT_EQ(entry.is_free(false), true);       
}
