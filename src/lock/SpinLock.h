/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SpinLock.h
 * Author: anhld2
 *
 * Created on July 7, 2017, 9:09 AM
 */

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>
#include <functional>
using namespace std;

/**
 * Class offers R/W locks using pthread
 * 
 */
template<typename T>
class SpinLock {
private:
    T _data;
    atomic_flag _locked;
public:
    SpinLock(T data);
    void lock();    
    void unlock();
    T read();
    /**
     * Write new data
     * @param data  Data to write
     * @param callback [Optional] A function of `void(T)` to handle old data
     */    
    void write(T data, const function<void(T&)>& callback = nullptr);
    ~SpinLock();
    SpinLock();
};

template<typename T>
T SpinLock<T>::read(){
    T res;
    this->lock();
    res = this->_data;
    this->unlock();
    return res;
}

template<typename T>
SpinLock<T>::~SpinLock(){
    
}

template<typename T>
void SpinLock<T>::write(T data, const function<void(T&)>& callback){
    T old_data;    
    this->wlock();
    old_data = this->_data;
    this->_data = data;
    if(callback != nullptr){ // invoke the callback if needed
        callback(old_data);
    }    
    this->unlock();
}


template<typename T>
SpinLock<T>::SpinLock(T data):_data(data), _locked(ATOMIC_FLAG_INIT) {

}

template<typename T>
SpinLock<T>::SpinLock():_locked(ATOMIC_FLAG_INIT) {

}

template<typename T>
void SpinLock<T>::lock() {
    while (this->_locked.test_and_set(memory_order_acquire)) { ; }
}

template<typename T>
void SpinLock<T>::unlock() {
    this->_locked.clear(memory_order_release);
}


#endif /* SPINLOCK_H */

