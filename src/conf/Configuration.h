/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Configuration.h
 * Author: anhld2
 *
 * Created on July 7, 2017, 2:59 PM
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <vector>
#include <map>
#include <memory>
#include <exception>
#include <string.h>
#include "../common.h"
using namespace std;

class Configuration {
private:
    map<string, shared_ptr<void>> _mapping;
    static Configuration _base;
    void merge_base();
public:
    void set(string key, const shared_ptr<void>& value);

    bool exists(string key);

    shared_ptr<void> get(string key) throw (invalid_argument);

    template<typename T>
    void set(string key, T* value);

    template<typename T>
    void set(string key, T value);    

    template<typename T>
    shared_ptr<T> get(string key) throw (invalid_argument);

    Configuration();
    Configuration(const Configuration& other);
    Configuration(Configuration&& other);
    Configuration& operator=(const Configuration&& other);

    static void set_base(Configuration& conf);
};

Configuration Configuration::_base;

void Configuration::merge_base(){
    this->_mapping.insert(
        Configuration::_base._mapping.begin(), 
        Configuration::_base._mapping.end()
    );
}

void Configuration::set_base(Configuration& conf){
    Configuration::_base = move(conf);
}

Configuration::Configuration()
    : _mapping(Configuration::_base._mapping)
{
    
}

Configuration::Configuration(const Configuration& other) 
    :_mapping(other._mapping)
{
    this->merge_base();

}

Configuration::Configuration(Configuration&& other) 
{
    this->_mapping = move(other._mapping);
    this->merge_base();
}

Configuration& Configuration::operator=(const Configuration&& other) {
    if (this != &other) {
        this->_mapping = move(other._mapping);
    }
    return *this;
}

void Configuration::set(string key, const shared_ptr<void>& value) {
    this->_mapping[key] = value;
}

bool Configuration::exists(string key) {
    return this->_mapping.find(key) != this->_mapping.end();
}

shared_ptr<void> Configuration::get(string key) throw (invalid_argument) {
    if (!this->exists(key)) {
        throw invalid_argument("Invalid key");
    }
    return this->_mapping[key];
}

template<typename T>
shared_ptr<T> Configuration::get(string key) throw (invalid_argument) {
    return static_pointer_cast<T>(this->get(key));
}

template<typename T>
void Configuration::set(string key, T* value) {
    this->_mapping[key] = shared_ptr<T>(value);
}

template<typename T>
void Configuration::set(string key, T value){
    this->_mapping[key] = shared_ptr<T>(new T(value));
}

#endif /* CONFIGURATION_H */

