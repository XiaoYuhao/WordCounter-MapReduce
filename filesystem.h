#ifndef _FILESYSTEM_H
#define _FILESYSTEM_H
#include <assert.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <errno.h>  
#include <string.h>  
#include <fcntl.h> 
#include <sys/types.h>
#include <sys/stat.h> 
#include <stdlib.h>
#include <stdarg.h>
/*
class FileSystem
{
public:
    virtual int fsopen() = 0;
    virtual int fsread() = 0;
    virtual int fswrite() = 0;
    virtual int fsclose() = 0;
    virtual int fsseek() = 0;

    virtual ~FileSystem() = 0;
};
*/
class LocalFileSystem
{
private:
    LocalFileSystem(){};
    static LocalFileSystem* _fs;
public:
    static LocalFileSystem* create(){
        if(_fs==NULL){
            _fs = new LocalFileSystem;
        }
        return _fs;
    }
    int fsopen(FILE **file, const char *path, const char *mode);
    int fsread(void *ptr, size_t size, size_t nitems, FILE *stream);
    int fswrite(const void *ptr, size_t size, size_t nitems, FILE *stream);
    int fsclose(FILE *stream);
    int fsseek(FILE *stream, long int offset, int whence);
    int fsscanf(FILE *stream, char *format, ... );
    int fsprintf(FILE *stream, char *format, ... );

    ~LocalFileSystem(){}
};

#endif