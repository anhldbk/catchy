/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DiskCache.h
 * Author: anhld2
 *
 * Created on June 28, 2017, 1:49 PM
 */

#ifndef DISKCACHE_H
#define DISKCACHE_H
#include <vector>
#include <exception>
#include <memory>
#include <stdint.h>
#include <string.h>
#include <sstream>
#include "Region.h"
#include "Fragment.h"
#include "RegionManager.h"
#include "utils.h"
#include "common.h"
#include "conf/ConfigurationReader.h"
#include "downloader/inc.h"
#include "logging/inc.h"
#include "FragmentTable.h"
#include <math.h>
using namespace std;

typedef vector<Fragment*> FragmentList;

class DiskCache {
private:
    shared_ptr<DownloaderBase> _downloader_ptr;
    shared_ptr<LoggerBase> _logger_ptr;
    static shared_ptr<DiskCache> _instance_ptr;
    FragmentTable* _fragment_tbl_ptr;
    DiskCache();

    // reload cache on configuration changes
    void _on_conf_changed(Configuration& conf) throw (runtime_error);
    void _load_regions(Configuration& conf) throw (runtime_error);
    void _load_tables(Configuration& conf) throw (runtime_error);

    /**
     * Free fragments for a specific region
     * @param region_ptr  Pointer to a region
     * @param min_fragments  Minimum fragments to free. Default is 1
     * @return Returns true if we can successfully free any fragments.
     */
    bool _free_fragments(Region* region_ptr, uint32_t min_fragments = 1);

public:
    ~DiskCache();

    static shared_ptr<DiskCache> get_instance();

    /**
     * Store a resource in the cache
     * @param resource  Resource to store
     * @return A session pointer where you can optionally register listeners for
     *  specific events. If the resource already exists, throw exceptions of
     *  `runtime_error` (you must invoke `remove(resource)` manually.
     */
    shared_ptr<Session> store(const Resource& resource) throw (runtime_error);

    /**
     * Get fragments of a resource 
     * @param resource  Resource
     * @param range Range (from, to)
     * @return Minimum list of fragments containing the range. If there's no 
     *  such resource, throw exceptions of `runtime_error`
     */
    FragmentList fetch(const Resource& resource, const Range& range) throw (runtime_error, invalid_argument);

    /**
     * Get all fragments of a resource 
     * @param resource  Resource
     * @return List of fragments. If there's no such resource, throw exceptions 
     *  of `runtime_error`
     */
    FragmentList fetch(const Resource& resource) throw (runtime_error);

    /**
     * Query information about a resource
     * @param resource  Resource
     * @return Resource meta. If there's no such resource, throw exceptions 
     *  of `runtime_error`
     */
    Resource query(const Resource& resource) throw (runtime_error);

    /**
     * Remove a resource
     * @param resource  Resource
     * @return Nothing. If there's no such resource, throw exceptions 
     *  of `runtime_error`
     */
    void remove(const Resource& resource) throw (runtime_error);

    /**
     * Initialize this cache by fetching configurations and initializing components
     * @return Nothing. if there's something wrong, throw exceptions of `runtime_error`
     */
    void init() throw (runtime_error);

};

bool DiskCache::_free_fragments(Region* region_ptr, uint32_t min_fragments) {
    if (region_ptr == nullptr) {
        return false;
    }

    uint32_t prev_capacity = region_ptr->fragment_capacity, current_capacity, delta;
    uint64_t key;
    vector<uint32_t> fragment_indices;
    while (true) {
        try {
            if (min_fragments == 0) {
                return true;
            }
            key = region_ptr->free_oldest();
            current_capacity = region_ptr->fragment_capacity;

            if (!this->_fragment_tbl_ptr->free_entry(key, fragment_indices)) {
                return false;
            }

            delta = prev_capacity - current_capacity;
            if (delta == 0) {
                return false;
            }

            region_ptr->free_fragments(fragment_indices);

            // free fragments
            if (min_fragments < delta) {
                return true;
            }
            min_fragments -= delta;
        } catch (...) {
            return false;
        }
    }

}

DiskCache::~DiskCache() {
    if (this->_fragment_tbl_ptr != nullptr) {
        this->_fragment_tbl_ptr->release();
        this->_fragment_tbl_ptr = nullptr;
    }
}


shared_ptr<DiskCache> DiskCache::_instance_ptr = shared_ptr<DiskCache>(new DiskCache());

DiskCache::DiskCache() : _logger_ptr(Logger::get_instance(LOGGER_NAME)), _fragment_tbl_ptr(nullptr), _downloader_ptr(Downloader::get_instance()) {
    auto handler = bind(&DiskCache::_on_conf_changed, this, placeholders::_1);
    ConfigurationReader::get_instance()->on_changed(handler);
}

shared_ptr<DiskCache> DiskCache::get_instance() {
    return _instance_ptr;
}

shared_ptr<Session> DiskCache::store(const Resource& resource) throw (runtime_error) {
    shared_ptr<Session> session_ptr = this->_downloader_ptr->download(resource);

    auto init_handler = [&](const shared_ptr<Resource>& res_ptr) {
        // resource now have `object_size` set
        _logger_ptr->debug(
                utils::str_format(
                "Initializing downloading session for resource % ...", session_ptr->resource.url
                )
                );
        uint64_t name_key = res_ptr->get_name_hash();
        shared_ptr<RegionManager> region_manager_ptr = RegionManager::get_instance();
        try {
            session_ptr->region_ptr = region_manager_ptr->query_region(
                    name_key,
                    res_ptr->object_size,
                    (FragmentType) res_ptr->fragment_type
                    );
        } catch (runtime_error err) {
            _logger_ptr->error("Can NOT get a dedicated region to store the resource");
            _logger_ptr->error(err.what());
            session_ptr->cancel();
            return;
        }

        FragmentType fragment_type = (FragmentType) session_ptr->region_ptr->fragment_type;
        uint32_t fragment_data_capacity = Fragment::calculate_data_capacity(fragment_type);

        // we don't know the index ahead of time
        // but we can update it later on
        uint32_t region_head = 0;
        uint32_t fragments_num = ceil(res_ptr->object_size / float(fragment_data_capacity));
        uint32_t fragment_capacity = session_ptr->region_ptr->fragment_capacity;

        while (true) {
            try {
                // reserve fragments indices first
                _logger_ptr->debug(
                        utils::str_format(
                        "Reserving % fragments for resource %...", fragments_num, session_ptr->resource.url
                        )
                        );
                vector<uint32_t> jmp_indices = session_ptr->region_ptr->reserve_fragments(name_key, fragments_num);
                vector<JmpDistance> jmps = session_ptr->region_ptr->get_jumps(jmp_indices);

                _fragment_tbl_ptr->reserve_entry(
                        name_key,
                        fragment_type,
                        region_head,
                        jmps
                        );
                break; // successfully reserved 
            } catch (...) {
                // TODO: Kick out older objects
                _logger_ptr->debug("Not enough fragments to allocate. Gonna free some ...");
                if (!this->_free_fragments(session_ptr->region_ptr, fragments_num)) {
                    _logger_ptr->error("Can't have any free fragments. Cancel the session now.");
                    session_ptr->cancel();
                    return;
                }
            }
        }

        _logger_ptr->info(
                utils::str_format("Starting to download resource at %...", session_ptr->resource.url)
                );
        session_ptr->start();
    };

    auto data_handler = [&](const shared_ptr<ResourceData>& data_ptr) {
        FragmentType fragment_type = (FragmentType) session_ptr->region_ptr->fragment_type;
        uint32_t fragment_data_capacity = Fragment::calculate_data_capacity(fragment_type);
        _logger_ptr->debug(
                utils::str_format("Receiving data for resource % at offset %...", session_ptr->resource.url, data_ptr->offset)
                );
        uint32_t fragment_index = ceil(data_ptr->offset / float(fragment_data_capacity));

        Fragment* fragment_ptr = Fragment::create_ptr(fragment_type);
        try {
            fragment_ptr->set_data(data_ptr->data);

            session_ptr->region_ptr->write_fragment(
                    fragment_index,
                    fragment_ptr
                    );
        } 
        catch(invalid_argument err){
            _logger_ptr->error("Resource data is too large to fit into dedicated fragments");
            _logger_ptr->error(err.what());           
        }
        catch (...) {
            _logger_ptr->error("Can NOT write fragments. Gonna cancel the download.");
            session_ptr->cancel();
        }

        Fragment::release_ptr(fragment_ptr);
    };

    auto end_handler = [&]() {
        _logger_ptr->debug(
            utils::str_format("Resource % is downloaded successfully", session_ptr->resource.url)
        );
    };

    auto error_handler = [&](const shared_ptr<SessionError>& error_ptr) {
        _logger_ptr->debug(
            utils::str_format("Error while downloading resource %", session_ptr->resource.url)
        );
    };

    session_ptr->on_init(init_handler);
    session_ptr->on_data(data_handler);
    session_ptr->on_end(end_handler);
    session_ptr->on_error(error_handler);

    return session_ptr;
}

FragmentList DiskCache::fetch(const Resource& resource, const Range& range) throw (runtime_error, invalid_argument) {
    uint64_t name_key = resource.get_name_hash();
    FragmentList result;
    shared_ptr<RegionManager> region_manager_ptr = RegionManager::get_instance();
    FragmentEntry* entry_ptr = this->_fragment_tbl_ptr->get_entry(name_key);
    if (entry_ptr == nullptr) {
        throw invalid_argument("No such resource");
    }

    FragmentType fragment_type = (FragmentType) entry_ptr->get_type();
    vector<uint32_t> fragment_indices = this->_fragment_tbl_ptr->get_fragment_indices(entry_ptr);
    uint32_t fragment_data_capacity = Fragment::calculate_data_capacity(fragment_type);
    Region* region_ptr = region_manager_ptr->get_region(name_key, fragment_type);

    uint32_t from_index = floor(range.from / float(fragment_data_capacity));
    uint32_t to_index = ceil(range.to / float(fragment_data_capacity));

    if (from_index >= fragment_indices.size() || to_index >= fragment_indices.size()) {
        throw invalid_argument("Invalid range");
    }

    for (uint32_t index = from_index; index < to_index + 1; index++) {
        Fragment* fragment_ptr = Fragment::create_ptr(fragment_type);
        result.push_back(
                region_ptr->read_fragment(
                fragment_indices[index],
                fragment_ptr
                )
                );
    }

    return result;
}

FragmentList DiskCache::fetch(const Resource& resource) throw (runtime_error) {
    uint64_t name_key = resource.get_name_hash();
    FragmentList result;
    shared_ptr<RegionManager> region_manager_ptr = RegionManager::get_instance();
    FragmentEntry* entry_ptr = this->_fragment_tbl_ptr->get_entry(name_key);
    if (entry_ptr == nullptr) {
        throw invalid_argument("No such resource");
    }

    FragmentType fragment_type = (FragmentType) entry_ptr->get_type();
    vector<uint32_t> fragment_indices = this->_fragment_tbl_ptr->get_fragment_indices(entry_ptr);
    uint32_t fragment_data_capacity = Fragment::calculate_data_capacity(fragment_type);
    Region* region_ptr = region_manager_ptr->get_region(name_key, fragment_type);

    for (uint32_t index = 0; index < fragment_indices.size(); index++) {
        Fragment* fragment_ptr = Fragment::create_ptr(fragment_type);
        result.push_back(
                region_ptr->read_fragment(
                fragment_indices[index],
                fragment_ptr
                )
                );
    }

    return result;
}

Resource DiskCache::query(const Resource& resource) throw (runtime_error) {

}

void DiskCache::remove(const Resource& resource) throw (runtime_error) {

}

void DiskCache::init() throw (runtime_error) {
    this->_logger_ptr->info("Initializing ...");
    ConfigurationReader::get_instance()->reload();
}

void DiskCache::_on_conf_changed(Configuration& conf) throw (runtime_error) {
    this->_load_regions(conf);
    this->_load_tables(conf);
}

void DiskCache::_load_regions(Configuration& conf) throw (runtime_error) {
    this->_logger_ptr->info("Loading regions ...");
    vector<RegionMeta> regions = *conf.get<vector < RegionMeta >> ("regions");
    RegionManager::get_instance()->reload(regions);
}

void DiskCache::_load_tables(Configuration& conf) throw (runtime_error) {
    TableMeta table_meta = *conf.get<TableMeta>("tables");

    this->_logger_ptr->info(
            utils::str_format("Loading tables from current directory of % ...\n", table_meta.file_path)
            );

    this->_fragment_tbl_ptr = FragmentTable::load(table_meta);
}


#endif /* DISKCACHE_H */

