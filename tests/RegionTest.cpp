/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "src/Region.h"
#include "src/Fragment.h"
#include "src/utils.h"
#include <bitset>
#include <gtest/gtest.h>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <stdio.h>
#include <thread>
#include <vector>
using namespace std;

char file_path[] = "./catchy_region_test";

void clean_up() { remove(file_path); }

TEST(RegionTest, testCreatePtr) {
  uint8_t fragment_type = 4;
  uint32_t fragment_capacity = 256;

  try {
    Region *region_ptr =
        Region::create_ptr(file_path, fragment_type, fragment_capacity);
    Region::release_ptr(region_ptr);
  } catch (runtime_error e) {
    cout << "Something goes wrong ?\n";
    cout << e.what() << endl;
  }

  clean_up();
}

TEST(RegionTest, testFromFile) {
  uint8_t fragment_type = 4;
  uint32_t fragment_capacity = 256;
  uint8_t fragment_num = 16;
  uint8_t buffer[] = "AAAAAAAAAAAA";
  uint64_t key = 0x5555;
  uint32_t size = sizeof(buffer);
  Region *region_ptr = nullptr;

  region_ptr = Region::create_ptr(file_path, fragment_type, fragment_capacity);
  vector<uint32_t> indices = region_ptr->reserve_fragments(key, fragment_num);
  EXPECT_EQ(indices.size(), fragment_num);
  EXPECT_EQ(region_ptr->fragment_avail, fragment_capacity - fragment_num);
  Region::release_ptr(region_ptr);

  region_ptr = Region::from_file(file_path);
  EXPECT_FALSE(region_ptr == nullptr);
  EXPECT_EQ(region_ptr->fragment_capacity, fragment_capacity);
  EXPECT_EQ(region_ptr->fragment_avail, fragment_capacity - fragment_num);
  EXPECT_EQ(region_ptr->get_time_table()->get_entry(0).key, key);

  clean_up();
}

TEST(RegionTest, testReadWrite) {
  uint8_t fragment_type = 4;
  uint32_t fragment_capacity = 256;
  uint8_t buffer[] = "AAAAAAAAAAAA";
  uint32_t size = sizeof(buffer);
  uint8_t buffer2[] = "BAAAAAAAAAAAA";
  uint32_t size2 = sizeof(buffer);
  Region *region_ptr = nullptr;
  Fragment *fragment_ptr = Fragment::create_ptr(fragment_type);

  try {
    fragment_ptr->set_data(buffer, size);
  } catch (invalid_argument e) {
    FAIL() << "Must set_data successfully";
  }

  try {
    region_ptr =
        Region::create_ptr(file_path, fragment_type, fragment_capacity);
    uint32_t fragment_index = region_ptr->_write_fragment(fragment_ptr);
    EXPECT_EQ(region_ptr->_is_free_fragment(fragment_index), false);

    // clear to check
    fragment_ptr->clear();
    fragment_ptr = region_ptr->read_fragment(
        fragment_index, (uint8_t *)fragment_ptr, fragment_ptr->get_size());
    EXPECT_EQ(fragment_ptr->get_data_size(), size);
    EXPECT_EQ(fragment_ptr->get_data()[0], buffer[0]);

    // write again with different data
    fragment_ptr->clear();
    fragment_ptr->set_data(buffer2, size2);
    region_ptr->write_fragment(fragment_index, fragment_ptr);

    // read again
    fragment_ptr->clear();
    fragment_ptr = region_ptr->read_fragment(fragment_index, fragment_ptr);
    EXPECT_EQ(fragment_ptr->get_data_size(), size2);
    EXPECT_EQ(fragment_ptr->get_data()[0], buffer2[0]);

    Fragment::release_ptr(fragment_ptr);
    Region::release_ptr(region_ptr);
  } catch (runtime_error e) {
    cout << "Something goes wrong ?\n";
    cout << e.what() << endl;
    FAIL() << "Failed to create a region file";
  }

  clean_up();
}

TEST(RegionTest, testWriteOverflow) {
  uint8_t fragment_type = 4;
  uint32_t fragment_capacity = 1;
  uint8_t buffer[] = "AAAAAAAAAAAA";
  uint32_t size = sizeof(buffer);
  Region *region_ptr = nullptr;
  Fragment *fragment_ptr = Fragment::create_ptr(fragment_type);

  fragment_ptr->set_data(buffer, size);
  region_ptr = Region::create_ptr(file_path, fragment_type, fragment_capacity);
  uint32_t fragment_index = region_ptr->_write_fragment(fragment_ptr);

  try {
    fragment_index = region_ptr->_write_fragment(fragment_ptr);
    FAIL() << "Must have exceptions";
  } catch (runtime_error e) {
  }
  EXPECT_EQ(region_ptr->fragment_avail, 0);

  Fragment::release_ptr(fragment_ptr);
  Region::release_ptr(region_ptr);

  clean_up();
}

TEST(RegionTest, testWriteConcurrent) {
  uint8_t fragment_type = 4;
  uint32_t fragment_capacity = 25;
  uint8_t buffer[] = "AAAAAAAAAAAA";
  uint32_t size = sizeof(buffer);
  Region *region_ptr = nullptr;
  Fragment *fragment_ptr = Fragment::create_ptr(fragment_type);

  try {
    fragment_ptr->set_data(buffer, size);
  } catch (invalid_argument e) {
    FAIL() << "Must set_data successfully";
  }
  uint32_t loop = 5, threads = 5;
  auto callback = [&]() {
    for (uint32_t index = 0; index < loop; index++) {
      region_ptr->_write_fragment(fragment_ptr);
    }
  };

  try {
    region_ptr =
        Region::create_ptr(file_path, fragment_type, fragment_capacity);
    utils::run_in_pool(callback, threads);
    EXPECT_EQ(region_ptr->fragment_avail, fragment_capacity - loop * threads);

    // test for written data
    for (uint32_t index = 0; index < fragment_capacity; index++) {
      fragment_ptr->clear();
      Fragment *clone_ptr = region_ptr->read_fragment(index, fragment_ptr);
      EXPECT_EQ(clone_ptr->get_data_size(), size);
      EXPECT_EQ(clone_ptr->get_data()[0], 'A');
    }

    Fragment::release_ptr(fragment_ptr);
    Region::release_ptr(region_ptr);
  } catch (runtime_error e) {
    cout << "Something goes wrong ?\n";
    cout << e.what() << endl;
    FAIL() << "Failed to create a region file";
  }

  clean_up();
}

TEST(RegionTest, testReadWriteConcurrent) {
  uint8_t fragment_type = 4;
  uint32_t fragment_capacity = 256;
  uint8_t buffer[] = "AAAAAAAAAAAA";
  uint32_t size = 12;
  Region *region_ptr = nullptr;
  Fragment *fragment_ptr = Fragment::create_ptr(fragment_type);

  try {
    fragment_ptr->set_data(buffer, size);
  } catch (invalid_argument e) {
    FAIL() << "Must set_data successfully";
  }
  uint32_t loop = 5, threads = 5;
  auto callback = [&]() {
    vector<uint32_t> indices;
    for (uint32_t index = 0; index < loop; index++) {
      indices.push_back(region_ptr->_write_fragment(fragment_ptr));
    }
    region_ptr->free_fragments(indices);
  };

  try {
    region_ptr =
        Region::create_ptr(file_path, fragment_type, fragment_capacity);
    utils::run_in_pool(callback, threads);
    EXPECT_EQ(region_ptr->fragment_avail, fragment_capacity);

    Fragment::release_ptr(fragment_ptr);
    Region::release_ptr(region_ptr);
  } catch (runtime_error e) {
    cout << "Something goes wrong ?\n";
    cout << e.what() << endl;
    FAIL() << "Failed to create a region file";
  }

  clean_up();
}

TEST(RegionTest, testReserveFragments) {
  uint8_t fragment_type = 4;
  uint32_t fragment_capacity = 10;
  uint32_t fragment_num = 6;
  uint8_t buffer[] = "AAAAAAAAAAAA";
  uint32_t size = 12;
  Region *region_ptr = nullptr;
  Fragment *fragment_ptr = Fragment::create_ptr(fragment_type);

  fragment_ptr->set_data(buffer, size);
  region_ptr = Region::create_ptr(file_path, fragment_type, fragment_capacity);

  uint64_t key1 = 0x5555, key2 = 0x6666, key3 = 0x7777;

  vector<uint32_t> indices = region_ptr->reserve_fragments(key1, fragment_num);
  EXPECT_EQ(indices.size(), fragment_num);
  EXPECT_EQ(region_ptr->fragment_avail, fragment_capacity - fragment_num);
  // check TimeEntry
  EXPECT_EQ(region_ptr->get_time_table()->get_entry(0).key, key1);

  try {
    region_ptr->reserve_fragments(key2, fragment_num);
    FAIL() << "Must have exceptions";
  } catch (runtime_error e) {
  }
  EXPECT_EQ(region_ptr->fragment_avail, fragment_capacity - fragment_num);

  region_ptr->reserve_fragments(key3, fragment_capacity - fragment_num);
  EXPECT_EQ(region_ptr->is_full(), true);
  EXPECT_EQ(region_ptr->get_time_table()->get_entry(1).key, key3);

  Fragment::release_ptr(fragment_ptr);
  Region::release_ptr(region_ptr);

  clean_up();
}

TEST(RegionTest, testReserveFragmentsFull) {
  uint8_t fragment_type = 4;
  uint32_t fragment_capacity = 10;
  uint32_t fragment_num = 6;

  Region *region_ptr = nullptr;
  region_ptr = Region::create_ptr(file_path, fragment_type, fragment_capacity);

  uint64_t key1 = 0x5555, key2 = 0x6666, key3 = 0x7777;

  vector<uint32_t> indices = region_ptr->reserve_fragments(key1, fragment_num);
  EXPECT_EQ(indices.size(), fragment_num);
  EXPECT_EQ(region_ptr->fragment_avail, fragment_capacity - fragment_num);
  // check TimeEntry
  EXPECT_EQ(region_ptr->get_time_table()->get_entry(0).key, key1);

  try {
    region_ptr->reserve_fragments(key2, fragment_num);
    FAIL() << "Must have exceptions";
  } catch (runtime_error e) {
    //    region_ptr->get_time_table()->free_entry();
  }
  EXPECT_EQ(region_ptr->fragment_avail, fragment_capacity - fragment_num);

  region_ptr->reserve_fragments(key3, fragment_capacity - fragment_num);
  EXPECT_EQ(region_ptr->is_full(), true);
  EXPECT_EQ(region_ptr->get_time_table()->get_entry(1).key, key3);

  EXPECT_EQ(region_ptr->free_oldest(), key1);
  region_ptr->free_fragments(indices);
  EXPECT_EQ(region_ptr->is_full(), false);

  Region::release_ptr(region_ptr);

  region_ptr = Region::from_file(file_path);

  clean_up();
}
