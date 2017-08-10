/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Resource.h
 * Author: anhld2
 *
 * Created on June 30, 2017, 1:53 PM
 */

#ifndef DOWNLOADERCOMMON_H
#define DOWNLOADERCOMMON_H

#include <string.h>
#include <stdint.h>
#include "../common.h"
#include "../hash/inc.h"
using namespace std;

#define SIZE_NA         0
#define NULL_KEY        0
#define EVENT_INIT      "init"
#define EVENT_DATA      "data"
#define EVENT_END       "end"
#define EVENT_ERROR     "error"
#define EVENT_START     "start"
#define EVENT_STOP      "stop"
#define EVENT_CANCEL    "cancel"
#define EVENT_RESUME    "resume"
#define EVENT_DONE      "done"

class Resource {
public:
    string url;
    uint8_t fragment_type;
    uint64_t object_size;   

    Resource() : object_size(SIZE_NA), fragment_type(FragmentDynamic) {

    }     

    uint64_t get_name_hash() const {
        return get_xxhash(url.c_str(), url.size());
    }
};

class ResourceData {
public:
    uint64_t offset;
    string data;
};

class SessionError {
public:
    uint64_t code;
    string message;
};



#endif /* DOWNLOADERCOMMON_H */

