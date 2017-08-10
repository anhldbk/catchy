/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   local-downloader.h
 * Author: anhld2
 *
 * Created on July 14, 2017, 1:44 PM
 */

#ifndef LOCAL_DOWNLOADER_H
#define LOCAL_DOWNLOADER_H

#include <memory>
#include <thread>
#include "session.h"
#include "base.h"
#include "common.h"
using namespace std;

/**
 * A single-threaded local-file downloader
 */
class LocalDownloader : public DownloaderBase {
private:
    static shared_ptr<LocalDownloader> _instance;
    LocalDownloader();
    
public:
    ~LocalDownloader();
    static shared_ptr<LocalDownloader> get_instance();
    shared_ptr<Session> download(const Resource& resource) throw (runtime_error) override;
};

shared_ptr<LocalDownloader> LocalDownloader::_instance = shared_ptr<LocalDownloader>(new LocalDownloader());

shared_ptr<LocalDownloader> LocalDownloader::get_instance() {
    return _instance;
}

LocalDownloader::LocalDownloader(): DownloaderBase() {

}

LocalDownloader::~LocalDownloader() {

}

shared_ptr<Session> LocalDownloader::download(const Resource& resource) throw (runtime_error) {
    shared_ptr<Session> session_ptr(new Session());
    
    
    return session_ptr;
}

#endif /* LOCAL_DOWNLOADER_H */

