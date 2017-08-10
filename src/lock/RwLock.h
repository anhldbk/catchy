/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Lockable.h
 * Author: anhld2
 *
 * Created on June 30, 2017, 3:49 PM
 */

#ifndef LOCKABLE_H
#define LOCKABLE_H
#include <pthread.h>
#include <functional>
using namespace std;

/**
 * Class offers R/W locks using pthread
 * 
 */
template<typename T>
class RwLock {
private:
    T _data;
public:
    RwLock(T data);
    ~RwLock();
    RwLock();    

    void rlock();    
    void wlock();    
    void unlock();

    T read();

    /**
     * Write new data
     * @param data  Data to write
     * @param callback A function of `void(T)` to handle old data
     */
    void write(T data, const function<void(T&)>& callback = nullptr);
private:
    pthread_rwlock_t _lock = PTHREAD_RWLOCK_INITIALIZER;
};

template<typename T>
T RwLock<T>::read(){
    T res;
    this->rlock();
    res = this->_data;
    this->unlock();
    return res;
}

template<typename T>
void RwLock<T>::write(T data, const function<void(T&)>& callback){
    T old_data;    
    this->wlock();
    this->_data = data;
    if (callback != nullptr) { // invoke the callback if needed
        callback(old_data);
    }    
    this->unlock();
}


template<typename T>
RwLock<T>::RwLock(T data) {
    this->_data = data;
}

template<typename T>
RwLock<T>::RwLock() {

}

template<typename T>
RwLock<T>::~RwLock() {
    pthread_rwlock_destroy(&this->_lock);

}

template<typename T>
void RwLock<T>::rlock() {
    if (pthread_rwlock_rdlock(&this->_lock) != 0) {
        perror("!> pthread_rwlock_rdlock error");
        exit(__LINE__);
    }
}

template<typename T>
void RwLock<T>::wlock() {
    if (pthread_rwlock_wrlock(&this->_lock) != 0) {
        perror("!> pthread_rwlock_rdlock error");
        exit(__LINE__);
    }
}

template<typename T>
void RwLock<T>::unlock() {
    if (pthread_rwlock_unlock(&this->_lock) != 0) {
        perror("!> pthread_rwlock_unlock error");
        exit(__LINE__);
    }
}

#endif /* LOCKABLE_H */

