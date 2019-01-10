
#include <string>
#include <unordered_map>
#include <memory>
#include <fstream>
#include "util_block_queue.hpp"

using std::string;
using std::shared_ptr;
using std::vector;
using wshutil::BlockBoundQueue;

namespace sophon_ick {

#define MODE_SET "set"
//#define MODE_BAG "bag"
#define OP_JOIN "join"
#define OP_SUBTRACT "subtract"

class IckChecker; // pre-defined

const static int _QUE_BOUND = 128;
const static int _MAP_CAP = 1000;
const static string _END_STR("");

void build_map(shared_ptr<IckChecker> checker);
void check_map(shared_ptr<IckChecker> checker);

class IckChecker {
public:
    IckChecker (const IckChecker &) = delete;
    IckChecker & operator = (IckChecker &) = delete;

    IckChecker (): IckChecker(_QUE_BOUND, _MAP_CAP) {}
    
    IckChecker (int qsize, int msize)  // queue bound, map capacity
            : stopped(false), build_cnt(0L), check_cnt(0L) { 
        //BlockBoundQueue<string> queue(qsize);
        BlockBoundQueue<shared_ptr<vector<string>>> queue(qsize);
        std::unordered_map<string, int32_t> map();
        this->map.reserve(msize);
        op = OP_JOIN;
        mode = MODE_SET;
    }

    // functions
    // not thread-safe
    void enque(const string & str) {
        //queue.push(str);
        const static size_t VSIZE = 2000;
        if(!cur_vec) {
            cur_vec = create_vec(VSIZE+1);
        }
        cur_vec->push_back(str);
        if(cur_vec->size() >= VSIZE) {
            queue.push(cur_vec);
            cur_vec = nullptr;
        }
    }

    // not thread-safe
    void deque_and_build() {
        //string s = queue.pop();
        shared_ptr<vector<string>> vec = queue.pop();
        if(!vec || vec->empty()) {
            stopped = true;
            return;
        }
        /*if(mode==MODE_BAG) {
            for(string s: *vec) {  
                if(map.find(s) == map.end()) {
                    map[s] = 1;
                } else {
                    map[s] += 1;
                }
            }
        }*/ 
        // MODE_SET
        for(string s: *vec) {  
            if(map.find(s) == map.end()) {
                map[s] = 1;
            }
        }
        build_cnt += vec->size();
    }

    // not thread-safe
    void deque_and_check() {
        shared_ptr<vector<string>> vec = queue.pop();
        if(!vec || vec->empty()) {
            stopped = true;
            return;
        }
        for(string s: *vec) {
            if(map.find(s) != map.end()) {
                map[s] -= 1;
            }
        }
        check_cnt += vec->size();
    }

    void record(std::ofstream & out) {
        if(op==OP_JOIN) {
            for (std::pair<string, int> it : map) {
                if(it.second <= 0) {
                    out << it.first << "\n" ;
                }
            }
        } else {
            for (std::pair<string, int> it : map) {
                if(it.second > 0) {
                    out << it.first << "\n" ;
                }
            }
        }
    }

    // not thread-safe
    void stop() {
        if(cur_vec && !cur_vec->empty()) {
            queue.push(cur_vec);
            cur_vec = create_vec(0);
        }
        queue.push(create_vec(0));
    }
    bool is_stop() {
        return stopped;
    }
    void reset_stop() {
        stopped = false;
    }
    string get_map_info() {
        string s = "";
        s += "map_size="+std::to_string(map.size());
        s += ", bucket_count="+std::to_string(map.bucket_count());
        s += ", load_factor="+std::to_string(map.load_factor());
        return s;
    }
    shared_ptr<vector<string>> create_vec(size_t reserve_size) {
        auto vec = std::make_shared<vector<string>>();
        vec->reserve(reserve_size);
        return vec;
    }
    void setOp(const string& op) { this->op = op; }
    void setMode(const string& mode) { this->mode = mode; }
    long get_build_cnt() { return build_cnt; }
    long get_check_cnt() { return check_cnt; }
    size_t get_map_size() { return map.size(); }
    

private:
    BlockBoundQueue<shared_ptr<vector<string>>> queue;
    shared_ptr<vector<string>> cur_vec;
    std::unordered_map<string, int32_t> map;
    volatile bool stopped;
    long build_cnt;
    long check_cnt;
    string op;
    string mode;
};
} // end of namespace


