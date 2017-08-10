/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   FragmentBuilder.h
 * Author: anhld2
 *
 * Created on June 26, 2017, 3:15 PM
 */

#ifndef FRAGMENTBUILDER_H
#define FRAGMENTBUILDER_H

#include <stdint.h>
#include <exception>
#include <string.h>
#include <string.h>
#include <atomic>
#include "Fragment.h"
using namespace std;

class FragmentBuilder{
private:
    FragmentBuilder(); // construct via `create` only
    Fragment* fragment_ptr = nullptr;
public:
    /**
     * Create a fragment builder from a specific type
     * type = 0 => this is a free entry
     * type = 0xF ==> this is a name entry
     * type = 0xE ==> this is a gonna-be-used entry
     * type [1 => 12] indicates fragment sizes     
     * @return Returns a builder
     */
    static FragmentBuilder create(uint8_t type) throw (invalid_argument);

    FragmentBuilder* set_data(uint8_t* data, uint32_t count);

    Fragment build() throw (invalid_argument);

    ~FragmentBuilder();
};

FragmentBuilder::FragmentBuilder(){
    Fragment::release_ptr(this->fragment_ptr);
}

FragmentBuilder FragmentBuilder::create(uint8_t type) throw (invalid_argument){
    if (type > 0xF) {
        throw invalid_argument("Invalid type. Must be in the range of [0, 0xF].");
    }

    FragmentBuilder builder;
    builder.fragment_ptr = Fragment::create_ptr(type);
    return builder;
}

FragmentBuilder* FragmentBuilder::set_data(uint8_t* data, uint32_t count) throw (invalid_argument){
    if(data == nullptr){
        throw invalid_argument("Invalid data buffer");
    }
    if(count > this->fragment_ptr->get_data_capacity()){
        throw invalid_argument("Data is too large");
    }

    uint8_t* data_ptr = this->fragment_ptr->get_data();
    memcpy(data_ptr, data, count);
    return this;
}

Fragment FragmentBuilder::build() throw (invalid_argument){
    return *this->fragment_ptr;
}


#endif /* FRAGMENTBUILDER_H */

