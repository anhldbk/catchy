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

#ifndef LOGGINGBASE_H
#define LOGGINGBASE_H
#include <string.h>
using namespace std;

enum LoggingLevel
{
    trace = 0,
    debug = 1,
    info = 2,
    warn = 3,
    error = 4,
    critical = 5,
    off = 6
};

class LoggerBase{
protected:
    string _name;
    LoggingLevel _level;
public:
    LoggerBase(const string& name, const LoggingLevel& level = LoggingLevel::error){
        this->_name = move(name);
        this->_level = level;
    }

    virtual void set_level(const LoggingLevel& level = LoggingLevel::error) = 0;

    virtual LoggingLevel get_level() const = 0;

    virtual void debug(const string& message) = 0;

    virtual void error(const string& message) = 0;

    virtual void info(const string& message) =0 ;

    virtual void warn(const string& message) = 0;
    
    virtual void critical(const string& message) = 0;

};

#endif /* LOGGINGBASE_H */

