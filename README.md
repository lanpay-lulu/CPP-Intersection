# CPP-Intersection
Stand alone intersection program for large scale data

## Background
Intersection of two set is very simple. Yet when the data scale is large, things are quite different. Some people tend to use hadoop to handle such tasks. However, hadoop or mapreduce is not designed to do intersecting because there is nothing to do with reduce. Holding large scale data in memory will often cause OOM exception. Further more, Java consumes much more memories than C++ when building maps, almost 3~4 times, resulting in worse performace.

## Introduction
This is a C++ project to calculate intersection of two data sets. It`s fast with multi-threads and consumes no more extra memory. For two sets with 100 million keys each (each key is of 8 bytes), it takse 35s for building map, and another 35s for intersection on my machine, which takes up to 9GB memory to do the test.


## Usage
- Use Scons to build project.
- Use liblog4cplus to log.
