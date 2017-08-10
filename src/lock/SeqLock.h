/*
 * Lockfree SeqLock
 * Reference:
 * http://www.1024cores.net/home/lock-free-algorithms/reader-writer-problem/improved-lock-free-seqlock
 */
/*
 * File:   SeqLock.h
 * Author: anhld2
 *
 * Created on June 30, 2017, 9:41 AM
 */

#ifndef SEQLOCK_H
#define SEQLOCK_H

#include <atomic>
#include <functional>
using namespace std;
#define POOL_SIZE 128

template <typename T> class SeqData {
public:
    int seq; // sequence number
    T data;

    SeqData(T value) : seq(0), data(value) {
    }

    SeqData() : seq(0) {
    }
};

template <typename T> 
class SeqLock {
private:
    SeqData<T> *_current_ptr;
    SeqData<T> _pool[POOL_SIZE];
    atomic_flag _lock_write;

    void lock();
    void unlock();
public:
    SeqLock(T data);

    /**
     * Read the protected data.
     * Becareful it's return a reference to the data. So if you just read 
     * (without any modification), you can use `T& data = this->read()` to 
     * improve performance. Otherwise, just `T data = this->read();` and then 
     * `this->write()`
     * @return Reference to the data
     */
    T& read();

    /**
     * Write new data
     * @param data  Data to write
     * @param callback [Optional] A function of `void(T)` to handle old data
     */
    void write(T data, const function<void(T&)>& callback = nullptr);
    ~SeqLock();
};

template <typename T> 
SeqLock<T>::SeqLock(T data): _current_ptr(nullptr), _lock_write(ATOMIC_FLAG_INIT) {
    this->write(data);
}

template <typename T> 
SeqLock<T>::~SeqLock() {
}

template <typename T> 
T& SeqLock<T>::read() {
    int seq;
    while (true) {
        SeqData<T> *current_ptr = this->_current_ptr; // load-consume
        seq = current_ptr->seq;
        // load-load fence
        if (seq % 2 == 1)
            continue;
        return current_ptr->data;
    }
}

template <typename T> 
void SeqLock<T>::write(T data, const function<void(T&)>& callback) {
    this->lock();
    SeqData<T> *next_ptr = nullptr;
    if (this->_current_ptr == nullptr) {
        this->_current_ptr = this->_pool;
        next_ptr = this->_current_ptr;
    } else {
        int idx = (this->_current_ptr - this->_pool + 1) % POOL_SIZE;
        next_ptr = &this->_pool[idx];
    }
    T& old_data = this->_current_ptr->data;

    this->_current_ptr = next_ptr; // store-release    
    next_ptr->seq += 1; // start writing new object
    next_ptr->data = data;
    if (callback != nullptr) { // invoke the callback if needed
        callback(old_data);
    }
    next_ptr->seq += 1; // end writing new object
    this->unlock();
}

template<typename T>
void SeqLock<T>::lock() {
    while (this->_lock_write.test_and_set(memory_order_acquire)) { ; }
}

template<typename T>
void SeqLock<T>::unlock() {
    this->_lock_write.clear(memory_order_release);
}


#endif /* SEQLOCK_H */
