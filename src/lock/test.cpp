#include <sys/stat.h>
#include <atomic>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <map>
#include <iostream>
#include <string.h>
#include <thread>
#include "lock/RwLock.h"
#include "lock/SeqLock.h"
#include <vector>
# include <chrono>
#include <functional>
//#include "RegionManager.h"

using namespace std;
using  ns = chrono::nanoseconds;
using  ms = chrono::milliseconds;
using read_time = chrono::steady_clock ;

class IncOne{
private:
    uint32_t data = 0;
public:

    void set(int data) {
        this->data = data;
    }
    uint32_t get(){
        return this->data;
    }
};

using namespace std;

int testReadWrite() {
    cout << "***************************\n";
    cout << "!> Test ReadWrite\n";
    uint32_t MAX = 1000000;

    IncOne one;
    auto start = read_time::now(); //use auto keyword to minimize typing strokes :)
    for(uint32_t i = 0; i< MAX; i++){
        one.set(one.get() + 1);
    }
    auto end = read_time::now();
    auto diff = end - start;
    cout<<"Elapsed time is :  "<< chrono::duration_cast<ms>(diff).count()<<" ms (not thread-safe)"<<endl;

    RwLock<uint32_t> rw(0);
    start = read_time::now(); //use auto keyword to minimize typing strokes :)
    for(uint32_t i = 0; i< MAX; i++){
        rw.write(rw.read() + 1);
    }
    end = read_time::now();
    diff = end - start;
    cout<<"Elapsed time is :  "<< chrono::duration_cast<ms>(diff).count()<<" ms (with RwLock)"<<endl;

    SeqLock<uint32_t> seq(0);
    start = read_time::now(); //use auto keyword to minimize typing strokes :)
    for(uint32_t i = 0; i< MAX; i++){
        seq.write(seq.read() + 1);
    }
    end = read_time::now();
    diff = end - start;
    cout<<"Elapsed time is :  "<< chrono::duration_cast<ms>(diff).count()<<" ms (with SeqLock)"<<endl;    
    
    return 0;
}

int testRead() {
    cout << "***************************\n";
    cout << "!> Test Read\n";    
    uint32_t MAX = 1000000;
    
    IncOne one;
    auto start = read_time::now(); //use auto keyword to minimize typing strokes :)
    for(uint32_t i = 0; i< MAX; i++){
        one.get();
    }
    auto end = read_time::now();
    auto diff = end - start;
    cout<<"Elapsed time is :  "<< chrono::duration_cast<ms>(diff).count()<<" ms (not thread-safe)"<<endl;

    RwLock<uint32_t> rw(0);    
    start = read_time::now(); //use auto keyword to minimize typing strokes :)
    for(uint32_t i = 0; i< MAX; i++){
        rw.read();
    }
    end = read_time::now();
    diff = end - start;
    cout<<"Elapsed time is :  "<< chrono::duration_cast<ms>(diff).count()<<" ms (with RwLock)"<<endl;

    SeqLock<uint32_t> seq(0);
    start = read_time::now(); //use auto keyword to minimize typing strokes :)
    for(uint32_t i = 0; i< MAX; i++){
        seq.read();
    }
    end = read_time::now();
    diff = end - start;
    cout<<"Elapsed time is :  "<< chrono::duration_cast<ms>(diff).count()<<" ms (with SeqLock)"<<endl;    
    
    return 0;
}


int main() {
    testRead();
    testReadWrite();
    return 0;
}
