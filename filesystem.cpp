
#include"filesystem.h"

LocalFileSystem* LocalFileSystem::_fs = NULL;


int LocalFileSystem::fsopen(FILE **file, const char *path, const char *mode){
    *file = fopen(path, mode);
    if(!file)return -1;
    return 1;
}

int LocalFileSystem::fsread(void *ptr, size_t size, size_t nitems, FILE *stream){
    return fread(ptr, size, nitems, stream);
}

int LocalFileSystem::fswrite(const void *ptr, size_t size, size_t nitems, FILE *stream){
    return fwrite(ptr, size, nitems, stream);
}

int LocalFileSystem::fsclose(FILE *stream){
    return fclose(stream);
}

int  LocalFileSystem::fsseek(FILE *stream, long int offset, int whence){
    return fseek(stream, offset, whence);
}


