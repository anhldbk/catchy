/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   internal.h
 * Author: anhld2
 *
 * Created on July 11, 2017, 10:36 AM
 */

#ifndef INTERNAL_H
#define INTERNAL_H

#include <functional>
#include <map>
#include <memory>
#include <vector>
#include <algorithm>
#include <atomic>
#include <stdint.h>
#include <string.h>
#include <unordered_map>
#include <uvw.hpp>
#include "../lock/SeqLock.h"

using namespace std;
using std::unordered_multimap;

typedef function<void(const shared_ptr<void>&) > EventListener;

struct InternalEvent {
    string name;
    shared_ptr<void> data;

    InternalEvent(string name, const shared_ptr<void>& data) {
        this->name = name;
        this->data = data;
    }
};

struct InternalEmitter : uvw::Emitter<InternalEmitter> {

    void emit(string name, const shared_ptr<void>& data) {
        publish(InternalEvent(name, data));
    }
};

struct InternalListener {
    uint64_t id;
    EventListener listener;
    bool used_once;

    InternalListener(uint64_t id, EventListener handler, bool used_once = false) {
        this->id = id;
        this->listener = handler;
        this->used_once = used_once;
    }
};

typedef unordered_multimap<string, InternalListener> HandlerMap;
typedef SeqLock<HandlerMap> InternalListeners;

#endif /* INTERNAL_H */

