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

#ifndef CONFIGURATIOINREADER_H
#define CONFIGURATIOINREADER_H

#include <fstream>
#include <algorithm>
#include <iostream>
#include <vector>
#include <stdint.h>
#include <sstream>
#include <functional>
#include <json/json.h>
#include <string.h>
#include <errno.h>
#include "../common.h"
#include "../Watcher.h"
#include "../lock/SpinLock.h"
#include "Configuration.h"
#include "../utils.h"
using namespace std;

// max fragments is 2^24
#define MAX_FRAGMENTS 16777216
#define MAX_HEADS 1048576
#define MAX_TAILS 65536
// max jumps is 2^20
#define MAX_JUMPS 1048576 

const char* CONFIG_FILE = "./conf.json";
typedef Json::Value Value;

typedef function<void(Configuration&) > changed_handler;

class ConfigurationReader {
public:
    static ConfigurationReader* get_instance();
    void on_changed(changed_handler handler);
    ~ConfigurationReader();
    void reload();
    void dispose();

private:
    ConfigurationReader();
    void _on_conf_changed(const char* file_name);
    bool _reload();
    changed_handler _handler;
    std::ifstream _file;
    Json::Value _root;
    static ConfigurationReader* _instance_ptr;
    SpinLock<bool> _guard;
    void _read(Configuration& conf) throw (invalid_argument);
    uint64_t _parse_size(string size) throw (invalid_argument);
    vector<RegionMeta> _read_regions() throw (invalid_argument);
    TableMeta _read_tables() throw (invalid_argument);
};

uint64_t ConfigurationReader::_parse_size(string size) throw (invalid_argument) {
    size_t pos = size.size() - 2;
    float number = stof(size.substr(0, pos));
    string unit = size.substr(pos, 2);
    std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);
    if (unit_shifts.find(unit) == unit_shifts.end()) {
        throw invalid_argument("Invalid unit. Currently only support `mb`, `gb`, `tb`.");
    }
    uint32_t shift = unit_shifts[unit];

    return number * (1l << shift);
}

ConfigurationReader* ConfigurationReader::_instance_ptr = new ConfigurationReader();

ConfigurationReader::ConfigurationReader() : _handler(nullptr), _file(std::ifstream(CONFIG_FILE)) {
    this->reload();

    auto handler = std::bind(&ConfigurationReader::_on_conf_changed, this, placeholders::_1);
    watch_start(CONFIG_FILE, handler);
}

ConfigurationReader::~ConfigurationReader() {
    this->dispose();
}

void ConfigurationReader::dispose(){
    watch_stop();
    this->_file.close();
}

void ConfigurationReader::on_changed(changed_handler handler) {
    this->_handler = handler;
}

ConfigurationReader* ConfigurationReader::get_instance() {
    return ConfigurationReader::_instance_ptr;
}

void ConfigurationReader::_on_conf_changed(const char* file_name) {
    this->reload();
}

bool ConfigurationReader::_reload() {
    Json::Reader reader;
    this->_file.clear();
    this->_file.seekg(0);

    if (!reader.parse(this->_file, this->_root, true)) {
        //for some reason it always fails to parse
        perror("!> Failed to parse configuration");
        perror(reader.getFormattedErrorMessages().c_str());
        return false;
    }

    if (this->_handler == nullptr) {
        return true;
    }

    Configuration conf;
    try{
        this->_read(conf);
    }
    catch(invalid_argument e){
        errno = EINVAL;
        perror(e.what());
        return false;
    }

    try{
        this->_handler(conf);        
    }
    catch(runtime_error e){
        errno = EAGAIN;
        perror(e.what());
        return false;        
    }

    return true;
}

void ConfigurationReader::reload() {
    this->_guard.lock();
    this->_reload();
    this->_guard.unlock();
}

vector<RegionMeta> ConfigurationReader::_read_regions() throw (invalid_argument){
    vector<RegionMeta> regions;
    auto regions_conf = this->_root["regions"];

    for (auto region_it = regions_conf.begin(); region_it != regions_conf.end(); region_it++) {
        const string fragment_type_str = region_it.key().asString();
        const FragmentType fragment_type = fragment_type_map[fragment_type_str];

        for (auto typed_region_it : regions_conf[fragment_type_str]) {
            if(typed_region_it["path"].isNull()){
                throw invalid_argument("!> Regions: Must specify attribute `path` for a Region in configuration");
            }
            RegionMeta meta;
            meta.fragment_type = fragment_type;
            meta.region_ptr = nullptr;
            meta.fragment_capacity = MAX_FRAGMENTS;
            
            meta.file_path = typed_region_it["path"].asString();
            utils::verify_dir(meta.file_path);
            if(meta.file_path.at(meta.file_path.size() -1) != '/'){
                meta.file_path.append("/");
            }
            meta.file_path.append("data.bin");
            
            if(typed_region_it["size"].isNull()){
                regions.push_back(meta);                
                continue; // no size specified, gonna use the max one
            }
            
            uint64_t max_size = this->_parse_size(typed_region_it["size"].asString());
            meta.fragment_capacity = max_size >> (9+fragment_type);
            if (meta.fragment_capacity > MAX_FRAGMENTS) {
                fprintf(stderr, "!> Regions: Maximum size is reached at Region %s\n", meta.file_path.c_str());
                throw invalid_argument("Invalid size");
            }
            regions.push_back(meta);
        }
    }
    return regions;
}

TableMeta ConfigurationReader::_read_tables() throw (invalid_argument){
    TableMeta meta;
    auto tables_conf = this->_root["tables"];    

    meta.head_capacity = tables_conf["heads"].asUInt();
    if(meta.head_capacity > MAX_HEADS){
        fprintf(stderr, "!> Tables: Maximum heads is reached");
        throw invalid_argument("Invalid heads");        
    }

    meta.tail_capacity = tables_conf["tails"].asUInt();
    if(meta.tail_capacity > MAX_TAILS){
        fprintf(stderr, "!> Tables: Maximum tails is reached");
        throw invalid_argument("Invalid tails");        
    }

    meta.jump_capacity = tables_conf["jumps"].asUInt();    
    if(meta.jump_capacity > MAX_JUMPS){
        fprintf(stderr, "!> Tables: Maximum jumps is reached");
        throw invalid_argument("Invalid jumps");        
    }

    string current_dir = utils::get_current_dir();
    current_dir.append("/tables");
    utils::verify_dir(current_dir);
    current_dir.append("/data.bin");
    meta.file_path = current_dir;

    return meta;
}

void ConfigurationReader::_read(Configuration& conf) throw (invalid_argument) {
    conf.set<string>("version", this->_root["version"].asString());
    conf.set<vector<RegionMeta>>("regions", this->_read_regions());   
    conf.set<TableMeta>("tables", this->_read_tables());       
}

#endif /* CONFIGURATIOINREADER_H */

