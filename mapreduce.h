#ifndef _MAPREDUCE_H
#define _MAPREDUCE_H
#include"filesystem.h"
#include <string>
#include <map>
#include <queue>

class Mapper
{
    //FILE *fp;
    std::map<std::string, int> mmap;
public:
    Mapper();
    virtual ~Mapper();
  
    virtual void Map(const std::string &input);
    virtual void Emit(const std::string &key, const std::string &value);
    virtual void save_map_result(const char *filename);
    virtual void Partition(const char *filename);
    virtual void Sort(const char *prefix, int nfile, const char *savefile);
};

class Reducer
{
    FILE *fp;
public:
    Reducer();
    virtual ~Reducer();

    virtual void Combine(const char *filename);
    virtual void Reduce(const std::string &key, const std::vector<std::string> &valist);
    virtual void Emit(const std::string &key, const std::string &value);
    virtual void set_config(const char *filename);
};

#define REGISTER_MAPPER(object) object obj; \
    Mapper_Point = &obj
#define REGISTER_REDUCER(object) Reducer_Point = &object
#endif

