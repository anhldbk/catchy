/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   RegionManager.h
 * Author: anhld2
 *
 * Created on June 30, 2017, 3:32 PM
 */

#ifndef REGIONMANAGER_H
#define REGIONMANAGER_H

#include <sys/stat.h>
#include <math.h>
#include <exception>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <stdint.h>
#include "Region.h"
#include "common.h"
#include "utils.h"
#include "lock/SeqLock.h"
#include "Region.h"
#include "HashRing.h"
#include "logging/inc.h"
using namespace std;

// Pre-calculated max fragment size (in bytes)

typedef HashNode<RegionMeta> RegionNode;
typedef HashRing<RegionMeta> RegionHashRing;
typedef shared_ptr<RegionHashRing> RegionHashRingPtr;
typedef map<FragmentType, RegionHashRingPtr> RegionsMap;

class RegionManager {
private:
    RegionManager();
    SeqLock<RegionsMap> _regions_map;
    static shared_ptr<RegionManager> _instance_ptr;

    void _clear();

public:
    /**
     * Guess the optimal fragment type for a specific object. 
     * The idea is can be described by the pseudo code below:
       -> for each region:
           max_fragment_size = max_size(region_type)
           fragment_avail = number_of_avaiable_fragments()

           acquired_fragment = ceil( object_size/max_fragment_size )
           last_fill_percent = (object_size % max_fragment_size)/max_fragment_size

           continue_if(fragment_avail < acquired_fragment)
           fragment_left = fragment_avail - acquired_fragment
       -> choose a type to maximize last_fill_percent * fragment_left
     * @param object_size   Size of the object (in bytes)
     * @return The optimal type. If there's no available region, throw exceptiosn of `runtime_error`
     */
    FragmentType guess_type(uint64_t object_size) throw (runtime_error);


    static shared_ptr<RegionManager> get_instance();
    ~RegionManager();
    void reload(vector<RegionMeta>& regions) throw (runtime_error);

    /**
     * Choose a dedicated region for a specific object
     *  This method should be used when you first store the object.
     * @param key   The object's key (may be a name-key or a content-key)
     * @param object_size   Size of the object (in bytes)
     * @param type  Fragment type. FragmentDynamic is used by default
     * @return 
     */
    Region* query_region(uint64_t key, uint64_t object_size, FragmentType type = FragmentDynamic) throw (runtime_error);

    /**
     * Get the region where an object MAY be previously stored. (Fragments may be added/removed at will)
     *  Note: the workflow should be: 
     *      + `query_region` -> get the region to write
     *      + `get_region` with specific key & type (obtained from FragmentTable) to read
     *      + Read the first fragment, verify if the same key is stored
     * @param key   The object's key (may be a name-key or a content-key)
     * @param type  Fragment type. It must not be FragmentDynamic. 
     * @return 
     */
    Region* get_region(uint64_t key, FragmentType type) throw (runtime_error);
};

shared_ptr<RegionManager> RegionManager::_instance_ptr(new RegionManager());

FragmentType RegionManager::guess_type(uint64_t object_size) throw (runtime_error) {
    float max_product = 0.0f, current_product, last_fill_percent;
    uint32_t acquired_fragment, fragment_avail, fragment_left;
    uint64_t max_fragment_size;
    FragmentType optimal_type = FragmentDynamic;
    RegionsMap& regions_map = this->_regions_map.read();

    for (auto it = regions_map.begin(); it != regions_map.end(); ++it) {
        max_fragment_size = max_fragment_size_map[it->first];
        acquired_fragment = ceil(object_size / float(max_fragment_size));
        last_fill_percent = (object_size % max_fragment_size) / float(max_fragment_size);

        // let's count all available fragments within this type of Regions
        fragment_avail = 0;
        auto regions = it->second->get_nodes();
        for (RegionNode region_node : regions) {
            RegionMeta region_meta = region_node.get_data();
            Region* region_ptr = (Region*)region_meta.region_ptr;
            fragment_avail += region_ptr->fragment_avail;
        }

        if (acquired_fragment > fragment_avail) {
            continue;
        }
        fragment_left = fragment_avail - acquired_fragment;
        current_product = fragment_left * last_fill_percent;
        if (current_product > max_product) {
            max_product = current_product;
            optimal_type = it->first;
        }
    }

    if (optimal_type == FragmentDynamic) {
        throw runtime_error("No suitable type");
    }

    return optimal_type;
}

RegionManager::RegionManager() : _regions_map(RegionsMap()) {
}

void RegionManager::_clear() {
    auto callback = [](RegionsMap& regions_map) {
        for (auto it = regions_map.begin(); it != regions_map.end(); ++it) {
            auto regions = it->second->get_nodes();
            for (RegionNode region_node : regions) {
                try {
                    Region::release_ptr((Region*)region_node.get_data().region_ptr);
                } catch (...) {

                }
            }
            it->second->clear();
        }

        regions_map.clear();
    };
    this->_regions_map.write(RegionsMap(), callback);
}

RegionManager::~RegionManager() {
    this->_clear();
}

shared_ptr<RegionManager> RegionManager::get_instance() {
    return _instance_ptr;
}

void RegionManager::reload(vector<RegionMeta>& regions) throw (runtime_error) {
    auto logger = Logger::get_instance("RegionManager");
    logger->info("Reloading...");
    this->_clear();

    RegionsMap regions_map;
    for (RegionMeta meta : regions) {
        auto it = regions_map.find(meta.fragment_type);
        if (it == regions_map.end()) {
            // no such ring, so create a new one
            regions_map[meta.fragment_type] = RegionHashRingPtr(new RegionHashRing());
        }

        meta.region_ptr = Region::load(meta);
        logger->info(utils::str_format(
            "Adding region at %...", meta.file_path
        ));
        RegionNode node(meta.file_path, meta);
        if (!regions_map[meta.fragment_type]->add_node(node)) {
            logger->info("Error");
            throw runtime_error("Can NOT add node into the Ring");
        }
        logger->info("Can you see me?");
    }
    this->_regions_map.write(regions_map);
}


Region* RegionManager::query_region(uint64_t key, uint64_t object_size, FragmentType type) throw (runtime_error) {
    if (type == FragmentDynamic) {
        type = this->guess_type(object_size);
    }
    return this->get_region(key, type);
}

Region* RegionManager::get_region(uint64_t key, FragmentType type) throw (runtime_error) {
    RegionsMap regions_map = this->_regions_map.read();
    RegionHashRingPtr region_hash_ring_ptr = regions_map[type];
    RegionNode* region_node_ptr = region_hash_ring_ptr->find_node(key);
    if (region_node_ptr == nullptr) {
        throw runtime_error("Can NOT get any region node");
    }
    
    return (Region*)region_node_ptr->get_data().region_ptr;
}


#endif /* REGIONMANAGER_H */

