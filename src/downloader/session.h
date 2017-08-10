/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Session.h
 * Author: anhld2
 *
 * Created on June 30, 2017, 1:55 PM
 */

#ifndef SESSION_H
#define SESSION_H

#include "../events/EventEmitter.h"
#include <memory>
#include <functional>
#include "common.h"
#include "../Region.h"
using namespace std;

typedef function<void(const shared_ptr<Resource>&)>     SessionInitHandler;
typedef function<void(const shared_ptr<ResourceData>&)>     SessionDataHandler;
typedef function<void()>                                    SessionEndHandler;
typedef function<void(const shared_ptr<SessionError>&)>     SessionErrorHandler;

class Session : public EventEmitter {
private:
    void _on_init(const shared_ptr<void>&);
    void _on_data(const shared_ptr<void>&);
    void _on_error(const shared_ptr<void>&);
    void _on_end(const shared_ptr<void>&);
    SessionInitHandler _init_handler;
    SessionDataHandler _data_handler;
    SessionEndHandler _end_handler;
    SessionErrorHandler _error_handler;

public:
    Resource resource;
    Region* region_ptr;    
    
    Session(const Resource& resource);

    void cancel();

    void start();

    void stop();

    void resume();

    void on_init(SessionInitHandler init_handler);

    void on_data(SessionDataHandler data_handler);

    void on_end(SessionEndHandler end_handler);

    void on_error(SessionErrorHandler error_handler);

    ~Session();
};

Session::Session(const Resource& resource) : EventEmitter() {
    this->resource = resource;
    this->region_ptr = nullptr;

    auto _on_init_bind = bind(&Session::_on_init, this, placeholders::_1);
    auto _on_data_bind = bind(&Session::_on_data, this, placeholders::_1);
    auto _on_end_bind = bind(&Session::_on_end, this, placeholders::_1);
    auto _on_error_bind = bind(&Session::_on_error, this, placeholders::_1);

    this->on(EVENT_INIT, _on_init_bind);
    this->on(EVENT_DATA, _on_data_bind);
    this->on(EVENT_END, _on_end_bind);
    this->on(EVENT_ERROR, _on_error_bind);

    this->_init_handler = nullptr;
    this->_data_handler = nullptr;
    this->_end_handler = nullptr;
    this->_error_handler = nullptr;    
}

void Session::cancel(){
    this->emit(EVENT_CANCEL);
}

void Session::start(){
    this->emit(EVENT_START);
}

void Session::stop(){
    this->emit(EVENT_STOP);
}

void Session::resume(){
    this->emit(EVENT_RESUME);
}

void Session::on_init(SessionInitHandler init_handler){
    this->_init_handler = init_handler;
}

void Session::on_data(SessionDataHandler data_handler){
    this->_data_handler = data_handler;
}

void Session::on_end(SessionEndHandler end_handler){
    this->_end_handler = end_handler;
}

void Session::on_error(SessionErrorHandler error_handler){
    this->_error_handler = error_handler;
}

void Session::_on_init(const shared_ptr<void>& data){
    if(this->_init_handler != nullptr){
        this->_init_handler(
            static_pointer_cast<Resource>(data)
        );
    }
}

void Session::_on_data(const shared_ptr<void>& data){
    if(this->_data_handler != nullptr){
        this->_data_handler(
            static_pointer_cast<ResourceData>(data)
        );
    }    
}

void Session::_on_error(const shared_ptr<void>& data){
    if(this->_error_handler != nullptr){
        this->_error_handler(
            static_pointer_cast<SessionError>(data)
        );
    }        
}

void Session::_on_end(const shared_ptr<void>& data){
    if(this->_end_handler != nullptr){
        this->_end_handler();
    }    
}

#endif /* SESSION_H */

