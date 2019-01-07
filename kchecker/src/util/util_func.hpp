#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sys/time.h>

namespace wshfunc{

size_t get_line_cnt(std::string & filename) {
    std::ifstream infile(filename);
    size_t cnt = std::count(std::istreambuf_iterator<char>(infile), 
             std::istreambuf_iterator<char>(), '\n');
    infile.close();
    return cnt;
}

bool is_file_empty(std::string & filename) {
	std::string line;
	std::ifstream fin(filename);
	while (std::getline(fin, line)) {
		if(line.length() > 2) {
			fin.close();
			return false;
		}
	}
	fin.close();
	return true;
}

size_t gettime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec*1000 + tv.tv_usec/1000;
}

std::vector<std::string> split(std::string str, std::string delimeter) {
	std::vector<std::string> vec;
	size_t startIndex = 0;
	size_t endIndex = 0;
	while( (endIndex = str.find(delimeter, startIndex)) < str.size() ) {
		std::string val = str.substr(startIndex, endIndex - startIndex);
		vec.push_back(val);
		startIndex = endIndex + delimeter.size();
	}
	if(startIndex < str.size()) {
		std::string val = str.substr(startIndex);
		vec.push_back(val);
	}
	return vec;
}

}; // end of namespace