#include <stdio.h>

#include "ick_conf.hpp"
#include "ick_checker.hpp"
#include "util_func.hpp"
#include "util_logger.h"
#include "cmdline.h"
#include <vector>
#include <fstream>
#include <functional>
#include <thread>

using std::vector;
using sophon_ick::IckChecker;


void get_cmd_arg(cmdline::parser & parser);

int main(int argc, char const *argv[])
{
    //== init logger
    log4cplus::Initializer initializer;
    log4cplus::BasicConfigurator config;
    log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT("conf/log4cplus.properties"));
    log4cplus::Logger logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT(LOGGER_NAME_NORMAL));

    LOG4CPLUS_INFO(logger, "index_checker start to run!");

    size_t begin_time = wshfunc::gettime();

    //== parse cmd args
    cmdline::parser argparser;
    get_cmd_arg(argparser);
    bool ok = argparser.parse(argc, argv);
    if(!ok) {
        printf("[Error] invalid input args. %s\n", argparser.usage().c_str());
        LOG4CPLUS_ERROR(logger, argparser.usage());
        return -1;
    }

    int Partition = argparser.get<int>("partition"); // bucket number

    string path = argparser.get<string>("path");
    string dfile = path+"/"+argparser.get<string>("dfile");

    size_t line_cnt;
    if(argparser.get<int>("dline") >= 0) {
        line_cnt = argparser.get<int>("dline");
    } else {
        //line_cnt = wshfunc::get_line_cnt(dfile);
        line_cnt = 10000000; // default 1kw
    } 
    LOG4CPLUS_INFO(logger, "suggested dbfile line count is " << line_cnt);

    // check cfiles line cnt
    //size_t cfiles_line_cnt = 0;
    
    string cfiles = argparser.get<string>("cfile");
    vector<string> cfile_vec = wshfunc::split(cfiles, ",");
    
    /*
    bool is_cfiles_empty = true;
    for(string f: cfile_vec) {
        string cfile = path+"/"+f;
        if(!wshfunc::is_file_empty(cfile)) {
            is_cfiles_empty = false;
            break;    
        }
    }
    LOG4CPLUS_INFO(logger, "cfiles is empty: " << is_cfiles_empty);
    if(is_cfiles_empty) { // just copy input file and return
        LOG4CPLUS_INFO(logger, "just copy!");
        string ofile = path+"/"+argparser.get<string>("ofile");
        std::ofstream outfile(ofile, std::ios::binary);
        std::ifstream src(dfile, std::ios::binary);
        outfile << src.rdbuf();
        outfile.close();
        return 0;
    }*/

    // Note: 1y keys takes about 9g memory to build map.
    const size_t MAX_MAP_CAPACITY = 150000000 / Partition; // total 1.5y
    line_cnt /= Partition;
    line_cnt += 20000;
    int MAP_CAPACITY;
    if(line_cnt < MAX_MAP_CAPACITY) {
        MAP_CAPACITY = line_cnt;
    } else {
        MAP_CAPACITY = MAX_MAP_CAPACITY;
    }
    LOG4CPLUS_INFO(logger, "MAP_CAPACITY is " << MAP_CAPACITY);

    //== init IckCheckers and store in vector.
    const int QUE_BOUND = 200;
    //const int MAP_CAPACITY = 100000;    
    vector<shared_ptr<IckChecker>> checker_vec;
    for(int i=0; i<Partition; i++) {
        checker_vec.push_back(shared_ptr<IckChecker>(new IckChecker(QUE_BOUND, MAP_CAPACITY)));
    }
    for(int i=0; i<Partition; i++) {
        checker_vec[i]->setOp(argparser.get<string>("op"));
        checker_vec[i]->setMode(argparser.get<string>("mode"));
    }
    
    //== init thread to build map
    vector<std::thread> build_thread_vec;    
    for(int i=0; i<Partition; i++) {
        build_thread_vec.push_back(
            std::thread(sophon_ick::build_map, checker_vec[i]));
    }

    size_t t = wshfunc::gettime();

    std::hash<string> hash_fn;
    string line;
    //== read build file
    size_t lcnt = 0;
    std::ifstream infile(dfile);
    while (std::getline(infile, line)) {
        if(line.length() < 1) { // ignore blank line
            continue;
        }
        lcnt += 1;
        size_t hash = hash_fn(line);
        int idx = hash % Partition;
        checker_vec[idx]->enque(line);
    }
    infile.close();
    LOG4CPLUS_INFO(logger, "read build file finished! cnt=" << lcnt);
    for(int i=0; i<Partition; i++) {
        checker_vec[i]->stop();
        build_thread_vec[i].join();
        checker_vec[i]->reset_stop();
    }
    t = wshfunc::gettime() - t;
    LOG4CPLUS_INFO(logger, "[time-stat] build map finished! took " << t);
    
    t = wshfunc::gettime();
    //== init thread to check map
    vector<std::thread> check_thread_vec;    
    for(int i=0; i<Partition; i++) {
        check_thread_vec.push_back(
            std::thread(sophon_ick::check_map, checker_vec[i]));
    }

    //== read cfiles
    for(string f: cfile_vec) {
        string cfile = path+"/"+f;
        LOG4CPLUS_INFO(logger, "handle cfile " << cfile);
        std::ifstream infile(cfile);
        while (std::getline(infile, line)) {
            if(line.length() <=1) { // ignore blank line
                continue;
            }
            size_t hash = hash_fn(line);
            int idx = hash % Partition;
            checker_vec[idx]->enque(line);
        }
        infile.close();
    }
    // check end
    for(int i=0; i<Partition; i++) {
        checker_vec[i]->stop();
        check_thread_vec[i].join();
    }
    t = wshfunc::gettime() - t;
    LOG4CPLUS_INFO(logger, "[time-stat] check map finished! took " << t);

    t = wshfunc::gettime();
    //== collect check failed rawkey into file
    string ofile = path+"/"+argparser.get<string>("ofile");
    std::ofstream outfile(ofile);
    //outfile.getloc();
    for(int i=0; i<Partition; i++) {
        checker_vec[i]->record(outfile);
    }
    outfile.close();
    t = wshfunc::gettime() - t;
    LOG4CPLUS_INFO(logger, "[time-stat] write to file finished! took " << t);

    size_t total_time = wshfunc::gettime() - begin_time;
    LOG4CPLUS_INFO(logger, "[time-stat] kjoin finished! total_time = " << total_time);
    
    return 0;
}


void get_cmd_arg(cmdline::parser & p) {
    p.add<string>("op", '\0', "join operator: join or subtract", false, OP_JOIN, 
        cmdline::oneof<string>(OP_JOIN,OP_SUBTRACT));
    p.add<string>("mode", '\0', "mode: bag or set", false, MODE_SET, 
        cmdline::oneof<string>(MODE_SET,MODE_SET));
    p.add<int>("partition", 'p', "partition number or bucket number", false, 2, cmdline::range(1, 8)); // 分桶数(每个桶一个线程)
    p.add<string>("path", '\0', "data path directory", false, "data");
    p.add<string>("dfile", '\0', "1st file name", false, "0.st");
    p.add<string>("cfile", '\0', "2nd file name", false, "0.nd");
    p.add<int>("dline", '\0', "suggeted dfile line cnt", false, -1);
    //p.add<string>("dfiles", '\0', "1st file list split by comma", false, "0.st,1.st");
    //p.add<string>("cfiles", '\0', "2nd file list split by comma", false, "0.nd,1.nd");
    p.add<string>("ofile", '\0', "output file", false, "0.out");
}


void cmdline_usage_example(int argc, const char * argv[]) {
    cmdline::parser a;
  // add specified type of variable.
  // 1st argument is long name
  // 2nd argument is short name (no short name if '\0' specified)
  // 3rd argument is description
  // 4th argument is mandatory (optional. default is false)
  // 5th argument is default value  (optional. it used when mandatory is false)
  a.add<string>("host", 'h', "host name", true, "");

  // 6th argument is extra constraint.
  // Here, port number must be 1 to 65535 by cmdline::range().
  a.add<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535));

  // cmdline::oneof() can restrict options.
  a.add<string>("type", 't', "protocol type", false, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"));

  // Boolean flags also can be defined.
  // Call add method without a type parameter.
  a.add("gzip", '\0', "gzip when transfer");

  // Run parser.
  // It returns only if command line arguments are valid.
  // If arguments are invalid, a parser output error msgs then exit program.
  // If help flag ('--help' or '-?') is specified, a parser output usage message then exit program.
  a.parse_check(argc, argv);

  // use flag values
  /*cout << a.get<string>("type") << "://"
       << a.get<string>("host") << ":"
       << a.get<int>("port") << endl; */

  // boolean flags are referred by calling exist() method.
  //if (a.exist("gzip")) cout << "gzip" << endl;
}

