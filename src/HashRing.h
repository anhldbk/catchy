/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HashRing.h, a C++ wrapper for lib hashring. Note: it's not thread safe.
 * Author: anhld2
 *
 * Created on July 3, 2017, 3:00 PM
 */

#ifndef HASHRING_H
#define HASHRING_H

#include <stdint.h>
#include <string.h>
#include <map>
#include <vector>
#include <iostream>
#include "hash/inc.h"
#include "hash/hash_ring.h"

#define REPLICAS_NUMBER 4096

using namespace std;

template<typename T>
class HashNode{
private:
    string _name;
    T _data;
public:
    HashNode(const string& name, T data){
        this->_name = move(name);
        this->_data = data;
    }   
    
    HashNode(): _name(""){
    }
    
    HashNode(HashNode<T>&& node): _name(move(node._name)), _data(node._data){

    }
    
    HashNode(const HashNode<T>& node):  _name(node._name), _data(node._data){
        
    }    
    
    HashNode<T>& operator=(HashNode<T>&& other) {
        // this may be called once or twice
        // if called twice, 'other' is the just-moved-from V subobject
        this->_name = move(other._name);
        this->_data = other._data;
        return *this;
    }    

    const string& get_name(){
        return this->_name;
    }

    const T& get_data(){
        return this->_data;
    }
};

enum HashFunction {
    SHA1 = 1,
    MD5,
    XXHASH
};

template<typename T>
class HashRing{
private:
    hash_ring_t* _ring;
    uint32_t _replicas_num;
    map<uint64_t, HashNode<T>> _nodes;
    HashFunction _hash_function;

public:
    HashRing(uint32_t replicas_num = REPLICAS_NUMBER, HashFunction hash_function = XXHASH);
    HashRing(const HashRing& other);
    HashRing(HashRing&& other);
    HashRing& operator=(const HashRing&& other);
    
    ~HashRing();
    bool add_node(HashNode<T> node);
    
    /**
     * Clear and allocate a blank ring
     */
    void reset();

    /**
     * Get all available nodes
     */    
    vector<HashNode<T>> get_nodes();

    HashNode<T>* find_node(uint64_t key);
    HashNode<T>* find_node(string name);

    bool remove_node(string name);

    /**
     * Clear the internal state. Deallocate all allocated memory
     */
    void clear();
};

template<typename T>
vector<HashNode<T>> HashRing<T>::get_nodes(){
    vector<HashNode<T>> result;
    for( auto it = this->_nodes.begin(); it != this->_nodes.end(); ++it ) {
        result.push_back( it->second );
    }    
    return result;
}

template<typename T>
HashRing<T>::HashRing(const HashRing& other){
    this->_ring = other._ring;
    this->_replicas_num = other._replicas_num;
    this->_nodes = other._nodes;
    this->_hash_function = other._hash_function;
}

template<typename T>
HashRing<T>::HashRing(HashRing&& other){
    this->_ring = other._ring;
    this->_replicas_num = other._replicas_num;
    this->_nodes = move(other._nodes);
    this->_hash_function = other._hash_function;
    other._ring = nullptr;
}

template<typename T>    
HashRing<T>& HashRing<T>::operator=(const HashRing&& other){
    if (this != &other) {
        this->_ring = other._ring;
        this->_replicas_num = other._replicas_num;
        this->_nodes = move(other._nodes);
        this->_hash_function = other._hash_function;
    }
    return *this;
}

template<typename T>
bool HashRing<T>::remove_node(string name){
    uint8_t* name_ptr = (uint8_t*)(name.c_str());
    size_t name_len = name.length();    

    bool result = hash_ring_remove_node(this->_ring, name_ptr, name_len) == HASH_RING_OK;
    if(!result){
        return false;
    }
    
    uint64_t name_hash = get_xxhash(name_ptr, name_len);    
    this->_nodes.erase(name_hash);
    
    return true;
}

template<typename T>
HashRing<T>::HashRing(uint32_t replicas_num, HashFunction hash_function){
    this->_replicas_num = replicas_num;
    this->_hash_function = hash_function;
    this->_ring = nullptr;
    this->reset();
}

template<typename T>
void HashRing<T>::clear(){
    if(this->_ring != nullptr){
        hash_ring_free(this->_ring);
        this->_ring = nullptr;
    }
    this->_nodes.clear();    
}

template<typename T>
void HashRing<T>::reset(){
    this->clear();
    this->_ring = hash_ring_create(this->_replicas_num, this->_hash_function);
}

template<typename T>
HashRing<T>::~HashRing(){
    this->clear();       
}

template<typename T>
bool HashRing<T>::add_node(HashNode<T> node){
    string name = node.get_name();
    uint8_t* name_ptr = (uint8_t*)(name.c_str());
    size_t name_len = name.length();
    
    uint64_t name_hash = get_xxhash(name_ptr, name_len);
    typename map<uint64_t, HashNode<T>>::iterator it = this->_nodes.find(name_hash);
    if(it != this->_nodes.end()){
        return false; // already exist
    }

    if(hash_ring_add_node(this->_ring, name_ptr, name_len) != HASH_RING_OK){
        return false;
    }
    this->_nodes[name_hash] = move(node);
    return true;
}

template<typename T>
HashNode<T>* HashRing<T>::find_node(uint64_t key){
    hash_ring_node_t* node = hash_ring_find_node(this->_ring, key);
    if(node == nullptr){
        return nullptr;
    }
    
    uint64_t node_hash = get_xxhash(node->name, node->nameLen);
    typename map<uint64_t, HashNode<T>>::iterator it = this->_nodes.find(node_hash);
    if(it != this->_nodes.end()){
        return &it->second;
    }
    return nullptr;
}

template<typename T>
HashNode<T>* HashRing<T>::find_node(string name){
    uint64_t key = get_xxhash((void*)name.c_str(), name.length());   
    return this->find_node(key);
}

#endif /* HASHRING_H */

