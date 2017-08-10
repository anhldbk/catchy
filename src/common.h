/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   common.h
 * Author: anhld2
 *
 * Created on June 30, 2017, 1:57 PM
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <string.h>
#include <map>
using namespace std;

#define LOGGER_NAME "catchy"

// Structure describing a range of bytes
struct Range {
    uint64_t from;
    uint64_t to;
};

enum FragmentType{
    FragmentDynamic = 0,
    Fragment1Kb,
    Fragment2Kb,    
    Fragment4Kb,    
    Fragment8Kb,
    Fragment16Kb,
    Fragment32Kb,    
    Fragment64Kb,
    Fragment128Kb,
    Fragment256Kb,
    Fragment512Kb,
    Fragment1mb,
    Fragment2mb,
};


class RegionMeta {
public:
    string file_path;
    FragmentType fragment_type;
    uint32_t fragment_capacity;
    void* region_ptr;
};

class TableMeta{
public:
    uint32_t head_capacity;
    uint32_t tail_capacity;
    uint32_t jump_capacity;
    string  file_path;
    void* tables_ptr;    
};

map<FragmentType, uint64_t> max_fragment_size_map = {
    std::pair <FragmentType, uint64_t> (Fragment1Kb, 1024),
    std::pair <FragmentType, uint64_t> (Fragment2Kb, 2048),
    std::pair <FragmentType, uint64_t> (Fragment4Kb, 4096),
    std::pair <FragmentType, uint64_t> (Fragment8Kb, 8192),
    std::pair <FragmentType, uint64_t> (Fragment16Kb, 16384),
    std::pair <FragmentType, uint64_t> (Fragment32Kb, 32768),
    std::pair <FragmentType, uint64_t> (Fragment64Kb, 65536),
    std::pair <FragmentType, uint64_t> (Fragment128Kb, 131072),
    std::pair <FragmentType, uint64_t> (Fragment256Kb, 262144),
    std::pair <FragmentType, uint64_t> (Fragment512Kb, 524288),
    std::pair <FragmentType, uint64_t> (Fragment1mb, 1048576),
    std::pair <FragmentType, uint64_t> (Fragment2mb, 2097152)
};

map<string, uint8_t> unit_shifts = {
    pair<string, uint8_t> ("mb", 20), // 1mb = (1<<20) bytes
    pair<string, uint8_t> ("gb", 30),  
    pair<string, uint8_t> ("tb", 40),  
};

map<string, FragmentType> fragment_type_map = {
    std::pair <string, FragmentType> ("1kb", Fragment1Kb),
    std::pair <string, FragmentType> ("2kb", Fragment2Kb),
    std::pair <string, FragmentType> ("4kb", Fragment4Kb),
    std::pair <string, FragmentType> ("8kb", Fragment8Kb),
    std::pair <string, FragmentType> ("16kb", Fragment16Kb),
    std::pair <string, FragmentType> ("32kb", Fragment32Kb),
    std::pair <string, FragmentType> ("64kb", Fragment64Kb),
    std::pair <string, FragmentType> ("128kb", Fragment128Kb),
    std::pair <string, FragmentType> ("256kb", Fragment256Kb),
    std::pair <string, FragmentType> ("512kb", Fragment512Kb),
    std::pair <string, FragmentType> ("1mb", Fragment1mb),
    std::pair <string, FragmentType> ("2mb", Fragment2mb),
};


#endif /* COMMON_H */

