/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Downloader.h
 * Author: anhld2
 *
 * Created on June 30, 2017, 2:33 PM
 */

#ifndef DOWNLOADERBASE_H
#define DOWNLOADERBASE_H

#include <memory>
#include "session.h"
#include <stdexcept>
#include "common.h"

class DownloaderBase{
public:
    DownloaderBase(){
        
    }
    
    virtual shared_ptr<Session> download(const Resource& resource) throw(runtime_error){
        throw runtime_error("Not implemented");
    }
};

#endif /* DOWNLOADERBASE_H */

