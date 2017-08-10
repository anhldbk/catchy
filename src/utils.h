/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   utils.h
 * Author: anhld2
 *
 * Created on June 17, 2017, 1:54 PM
 */

#ifndef UTILS_H
#define UTILS_H

#include <thread>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <sys/stat.h>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include <memory.h>
using namespace std;

namespace utils {

    /**
     * Execute a command and return output
     * @param cmd   Command to exec
     * @return Returns output (to console) 
     */
    string shell_exec(string cmd) throw (invalid_argument) {
        string data;
        FILE * stream;
        const int max_buffer = 256;
        char buffer[max_buffer];
        cmd.append(" 2>&1");

        stream = popen(cmd.c_str(), "r");
        if (stream) {
            while (!feof(stream))
                if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
            pclose(stream);
        }
        return data;
    }

    void run_in_pool(std::function<void() > routine, uint8_t max_thread = 4) {
        std::thread modifiers[max_thread];
        uint8_t i;

        for (i = 0; i < max_thread; i++) {
            modifiers[i] = std::thread(routine);
        }

        for (i = 0; i < max_thread; i++) {
            modifiers[i].join();
        }

    }

    uint16_t get_tag_12_bits(uint64_t key) {
        return static_cast<uint16_t> (key & 0xF);
    }

    uint32_t get_tag_24_bits(uint64_t key) {
        return (key << 12) >> 40;
    }

    uint32_t get_tag_28_bits(uint64_t key) {
        return (key << 5) & 0x0FFFFFFF;
    }

    /**
     * Just like `sleep()`
     * @param  seconds Number of seconds to sleep
     */
    void idle(uint32_t seconds) {
        struct timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 500000;
        select(0, NULL, NULL, NULL, &tv);
    }

    /**
     * Check if a file exists via its absolute path
     * @param path  The file path
     * @return Returns true if the file does exist. Otherwise, returns false.
     */
    inline bool file_exists(const string& path) {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }

    /**
     * Get the current working directory
     * @return The directory. For example, "/home/cache/catchy"
     */
    string get_current_dir() {
        char buf[FILENAME_MAX];
        char* succ = getcwd(buf, FILENAME_MAX);
        if (succ) return std::string(succ);
        return ""; // raise a flag, throw an exception, ..
    }

    // http://en.cppreference.com/w/cpp/language/parameter_pack

    string str_format(const char* format) // base function
    {
        ostringstream os;
        os << format;
        return os.str();
    }

    template<typename T, typename... Targs>
    string str_format(const char* format, T value, Targs... Fargs) // recursive variadic function
    {
        ostringstream os;
        for (; *format != '\0'; format++) {
            if (*format == '%') {
                os << value;
                os << str_format(format + 1, Fargs...); // recursive call
                return os.str();
            }
            os << *format;
        }
        return os.str();
    }

    /**
     * Verify if a directory exists. If it does NOT, create it on the fly
     */
    void verify_dir(const string& dir) throw (invalid_argument) {
        string command = str_format("/bin/mkdir -p %", dir);
        string output = shell_exec(command);
        if (output.size() != 0) {
            throw invalid_argument(output);
        }
    }
}


#endif /* UTILS_H */

