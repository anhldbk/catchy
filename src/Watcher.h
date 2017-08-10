/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Configuration.h
 * Author: anhld2
 *
 * Created on June 14, 2017, 4:56 PM
 */

#ifndef WATCHER_h
#define WATCHER_h

#include <sys/stat.h>
#include <atomic>
#include <iostream>
#include <functional>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <uv.h>
#include "lock/SpinLock.h"

using namespace std;
typedef const function<void(const char*) >& watch_callback;

struct WatchTarget {
    string dir;
    string file_name;
    bool is_file;

    bool _is_dir(const char * path) {
        struct stat sb;
        return (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode));
    }

    void parse(string path) {
        this->is_file = false;
        this->dir = path;
        if (this->_is_dir(path.c_str())) {
            return;
        }
        size_t sep = path.find_last_of("/");
        if (sep == std::string::npos) {
            // may be we're provided with a filename in the current directory only
            this->file_name = path;
            this->dir = ".";
        } else {
            this->file_name = path.substr(sep + 1, path.size() - sep - 1);
            this->dir = path.substr(0, sep);
        }
        this->is_file = true;
    }

    bool match_file(const char* file_name) {
        return (this->is_file) && (this->file_name.compare(file_name) == 0);
    }

    WatchTarget(string path = "") {
        this->parse(path);
    }
};

pthread_t _watch_thread;
bool _watch_running = false;
WatchTarget _watch_target;
bool _change_lock;
bool _file_toggled = false;
uv_loop_t* _watch_loop;
function<void(const char* file_name) > _watch_callback = nullptr;
SpinLock<bool> _guard;

void _watch_on_change(uv_fs_event_t *handle, const char *file_name, int events, int status) {
    if (_change_lock || (_watch_callback == nullptr)) return;
    if (!_watch_target.is_file) {
        return _watch_callback(file_name);
    }
    if (_watch_target.match_file(file_name)) {
        if (events == UV_CHANGE){
            // for some mysterious reason, 
            // if we write content to a file successfully, this method is called twice
            _file_toggled = !_file_toggled;
            if(!_file_toggled){
                _watch_callback(file_name);
                _change_lock = true;
            }
        }
    }
}

void _signal_handler(uv_signal_t *handle, int signum) {
    uv_signal_stop(handle);
}

void clear_change_lock(uv_timer_t* timer, int status) {
    _change_lock = false;
}

void *_watch_routine(void *arg) {
    // register signal watcher
    _watch_loop = uv_default_loop();
    uv_signal_t sig1a;
    uv_signal_init(_watch_loop, &sig1a);
    uv_signal_start(&sig1a, _signal_handler, SIGUSR1);

    uv_fs_event_t* fs_event_req = (uv_fs_event_t*) malloc(sizeof (uv_fs_event_t));
    uv_fs_event_init(_watch_loop, fs_event_req);
    // The recursive flag watches subdirectories too.
    uv_fs_event_start(fs_event_req, _watch_on_change, _watch_target.dir.c_str(), UV_FS_EVENT_RECURSIVE);

    uv_timer_t timer;
    uv_timer_init(_watch_loop, &timer);
    uv_timer_start(&timer, (uv_timer_cb) clear_change_lock, 0, 1000);

    uv_run(_watch_loop, UV_RUN_DEFAULT);
}

bool watch_start(string path, watch_callback callback) {
    bool result = false;
    _guard.lock();
    _watch_target.parse(path);
    _watch_callback = callback;
    result = !(_watch_running || pthread_create(&_watch_thread, NULL, _watch_routine, nullptr));
    if (result) {
        _watch_running = true;
    }
    _guard.unlock();
    return result;
}

/**
 * Stop watching the previously-watched file
 * @return Returns true if we can stop watching.
 */
bool watch_stop() {
    bool result = true;
    _guard.lock();
    if (_watch_running == true) {
        uv_stop(_watch_loop);
        uv_loop_close(_watch_loop);
        _watch_loop = nullptr;
        pthread_kill(_watch_thread, SIGINT);
        pthread_join(_watch_thread, NULL);
        _watch_running = false;
    } else {
        result = false;
    }
    _guard.unlock();
    return result;
}

#endif /* WATCHER_h */

