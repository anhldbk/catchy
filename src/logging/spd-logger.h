/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   base.h
 * Author: anhld2
 *
 * Created on July 10, 2017, 11:37 AM
 */

#ifndef SPDLOGGER_H
#define SPDLOGGER_H
#include "base.h"
#include <spdlog/spdlog.h>
#include <string.h>
#include <map>

using namespace std;
namespace spd = spdlog;

class SpdLogger : public LoggerBase {
private:
    std::shared_ptr<spdlog::logger> _logger;
    static map<string, shared_ptr<LoggerBase>> _instances;
public:

    SpdLogger(const string& name, const LoggingLevel& level = LoggingLevel::info) : LoggerBase(name, level) {
        this->_logger = spd::get(name);
        if (this->_logger == nullptr) {
            this->_logger = spd::stdout_color_mt(name);
        }
        this->set_level(level);
    }

    static shared_ptr<LoggerBase> get_instance(const string& name, const LoggingLevel& level = LoggingLevel::info) {
        if (SpdLogger::_instances.find(name) != SpdLogger::_instances.end()) {
            return SpdLogger::_instances[name];
        }
        shared_ptr<LoggerBase> logger(new SpdLogger(name, level));
        SpdLogger::_instances[name] = logger;
        return logger;
    }

    /** Sets the level of logging that Wintermute uses globally. */
    void set_level(const LoggingLevel &level) override {
        this->_logger->set_level((spd::level::level_enum)level);
        this->_level = level;
    }

    LoggingLevel get_level() const override {
        return this->_level;
    }

    void debug(const string &message) override {
        this->_logger->debug(message.c_str());
    }

    void error(const string &message) override {
        this->_logger->error(message.c_str());
    }

    void info(const string &message) override {
        this->_logger->info(message.c_str());
    }

    void warn(const string &message) override {
        this->_logger->warn(message.c_str());
    }

    void critical(const string &message) override {
        this->_logger->critical(message.c_str());
    }

    static void release() {
        // Release and close all loggers
        spd::drop_all();
    }
};

map<string, shared_ptr<LoggerBase>> SpdLogger::_instances;

#endif /* SPDLOGGER_H */
