#ifndef _EVENT_EMITTER_H_
#define _EVENT_EMITTER_H_

// This class implements a generic nodejs-inspired event-emitter .
// It's based on `uvw::Emitter` but with just a single event named 
// `InternalEvent`. If you want a more perofmant emitter, please 
//  use the original one with specific events

#include "./common.h"



class EventEmitter {
private:
    shared_ptr<InternalEmitter> _emitter_ptr;

    shared_ptr<InternalListeners> _listeners;
    atomic<uint64_t> _counter;

public:
    /**
     * Register a listener for a specific event
     * @param name  Name of the event
     * @param listener   Function to listener this kind of events
     * @param once   If set to `true`, the listener listence only once.
     * @return ID of the registered listener. Use it with `remove_listener`
     */
    uint64_t on(string name, const EventListener& listener, bool once = false);
    uint64_t once(string name, const EventListener& listener);
    void emit(string name, const shared_ptr<void>& data = nullptr);
    bool remove_listener(string name, uint64_t listener_id);
    bool remove_listeners(string name, vector<uint64_t> listener_ids);
    void remove_all_listeners();
    EventEmitter();
    ~EventEmitter();
};

EventEmitter::EventEmitter() : _counter(0),
    _listeners(new InternalListeners(HandlerMap())),
    _emitter_ptr(new InternalEmitter()) 
{

    this->_emitter_ptr->on<InternalEvent>([&](const InternalEvent& event, auto& emitter) {
        HandlerMap& listener_map = _listeners->read();
        vector<uint64_t> onces;
        auto range = listener_map.equal_range(event.name);
        for (auto it = range.first; it != range.second; ++it) {
            // let's invoke the listeners
            it->second.listener(event.data);
            if (it->second.used_once) {
                onces.push_back(it->second.id);
            }
        }
        this->remove_listeners(event.name, onces);
    });
}

EventEmitter::~EventEmitter(){
    this->remove_all_listeners();
    this->_emitter_ptr->clear();
}

uint64_t EventEmitter::on(string name, const EventListener& listener, bool once) {
    uint64_t id;
    do {
        id = this->_counter;
    } while (!this->_counter.compare_exchange_strong(id, id + 1));
    HandlerMap listener_map = this->_listeners->read();
    listener_map.insert({
        {name, InternalListener(id, move(listener), once)}
    });
    this->_listeners->write(listener_map);
    return id;
}

uint64_t EventEmitter::once(string name, const EventListener& listener){
    return this->on(name, listener, true);
}

void EventEmitter::emit(string name, const shared_ptr<void>& data) {
    this->_emitter_ptr->emit(name, data);
}

bool EventEmitter::remove_listener(string name, uint64_t listener_id) {
    HandlerMap listener_map = this->_listeners->read();
    bool found = false;
    auto range = listener_map.equal_range(name);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second.id == listener_id) {
            found = true;
            listener_map.erase(it);
            break;
        }
    }
    if (found) {
        this->_listeners->write(listener_map);
    }
    return found;
}

bool EventEmitter::remove_listeners(string name, vector<uint64_t> listener_ids) {
    HandlerMap listener_map = this->_listeners->read();
    bool found = false;
    auto range = listener_map.equal_range(name);

    for (auto it = range.first; it != range.second; ++it) {
        if (find(listener_ids.begin(), listener_ids.end(), it->second.id) != listener_ids.end()) {
            found = true;
            listener_map.erase(it);
        }
    }
    if (found) {
        this->_listeners->write(listener_map);
    }
    return found;
}

void EventEmitter::remove_all_listeners() {
    HandlerMap listener_map;
    this->_listeners->write(listener_map);
}

#endif
