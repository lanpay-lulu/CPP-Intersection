#include "ick_checker.hpp"
#include "util_logger.h"

/**
 * This function will consume the queue to build IckChecker`s map.
 * It`s suitable for a thread call.
*/
void sophon_ick::build_map(shared_ptr<sophon_ick::IckChecker> checker) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME_NORMAL));
    LOG4CPLUS_INFO(logger, "build_map() begin! map_info: " << checker->get_map_info());
    while(!checker->is_stop()) {
        checker->deque_and_build();
    }
    LOG4CPLUS_INFO(logger, "build_map() end! build_cnt=" << checker->get_build_cnt() << ", " << checker->get_map_info());
}

/**
 * This function will consume the queue to check IckChecker`s map.
 * It`s suitable for a thread call.
*/
void sophon_ick::check_map(shared_ptr<sophon_ick::IckChecker> checker) {
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME_NORMAL));
    LOG4CPLUS_INFO(logger, "check_map() begin!");
    while(!checker->is_stop()) {
        checker->deque_and_check();
    }
    LOG4CPLUS_INFO(logger, "check_map() end! check_cnt=" << checker->get_check_cnt());
}


