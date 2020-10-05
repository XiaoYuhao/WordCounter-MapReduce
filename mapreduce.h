#ifndef _MAPREDUCE_H
#define _MAPREDUCE_H
#include"filesystem.h"
#include<string>
#include<map>

class Mapper
{
    FILE *fp;
    //std::map<std::string, int> mmap;
public:
    Mapper();
    virtual ~Mapper();
  
    virtual void Map(const std::string input);
    virtual void Emit(const std::string &key, const std::string &value);
    virtual void set_config(const char *filename);
    virtual void Partition(const char *filename);
    
};

class Reducer
{
public:
    virtual void Reduce();
};

#define REGISTER_MAPPER(object) object obj; \
    Mapper_Point = &obj
#define REGISTER_REDUCER(object) Reducer_Point = &object
#endif

