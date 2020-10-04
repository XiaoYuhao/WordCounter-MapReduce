#ifndef _MAPREDUCE_H
#define _MAPREDUCE_H


class Mapper
{
public:
    virtual void Map();

};

class Reducer
{
public:
    virtual void Reduce();
};

#endif

