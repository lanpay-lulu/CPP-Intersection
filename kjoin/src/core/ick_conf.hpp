#pragma once

/**
 * This is the configure class that provides config loading and 
 * params initializing.
 * It uses a singleton model.
*/

#include <string>
#include "util_config.hpp"
#include "util_logger.h" 

using std::string;

class IckConf {
public:
    IckConf & getInstance();
    int init(string path);

private:
    IckConf();
    int init2(); // init params

    // non-copyable
    IckConf(const IckConf&) = delete;
    IckConf& operator=(const IckConf&) = delete;

public:
    // params
    

private:
    static IckConf instance;
    bool is_init;
    string conf_path;
    Config config;
    log4cplus::Logger logger;

};