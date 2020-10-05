#include"mapreduce.h"

LocalFileSystem *fs=LocalFileSystem::create();

Mapper::Mapper(){
    fp = NULL;
}

Mapper::~Mapper(){
    fs->fsclose(fp);
    fp = NULL;
}
void Mapper::Map(const std::string input){
    printf("???\n");
}

void Mapper::Emit(const std::string &key, const std::string &value){
    fs->fswrite(key.c_str(), key.length(), 1, fp);
    fs->fswrite(" ", 1, 1, fp);
    fs->fswrite(value.c_str(), value.length(), 1, fp);
    fs->fswrite("\n", 1, 1, fp);
}

void Mapper::Partition(const char *filename){
    FILE *ffp;
    fs->fsopen(&ffp, filename, "a+");
    

} 


void Mapper::set_config(const char *filename){
    fs->fsopen(&fp, filename, "a+");
}