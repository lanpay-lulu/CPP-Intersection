#include "ick_conf.hpp"

IckConf IckConf::instance;

IckConf::IckConf() {
    is_init = false;
    logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME_NORMAL));
}

IckConf & IckConf::getInstance() {
    return instance;
}

int IckConf::init(string path) {
    this->conf_path = path;
    try {
        if(!config.read(path)) {
            LOG4CPLUS_FATAL(logger, "Error reading config file! Can not open " << path);
            return -1;
        }
        is_init = true;
    } catch (std::invalid_argument& e) {
        LOG4CPLUS_FATAL(logger, "Error reading config file! e=" << e.what());
        return -2;
    }
    return init2();
}

int IckConf::init2() {
    // brokers
    // db topic
    // index topics
    
    // other settings
    return 0;
}





