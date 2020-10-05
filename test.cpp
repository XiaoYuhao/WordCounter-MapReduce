
#include"process_pool.h"

Mapper *Mapper_Point = NULL;
Reducer *Reducer_Point = NULL;

bool isspace(char c){
    if(c==' ')return true;
    else return false;
}

class WordCounter:public Mapper
{
public:
    virtual void Map(const std::string input){
        const int n = input.length();
        for(int i=0;i<n;){
            while((i<n)&&isspace(input[i]))i++;
            int start = i;
            while((i<n)&&!isspace(input[i]))i++;
            if(start<i){
                Emit(input.substr(start,i-start), "1");
            }
        }
    }
};


int main(){
    WordCounter obj;
    Mapper_Point = &obj;

    process_pool* pool=process_pool::create(10);
    pool->run();
    return 0;
}

/*
#include"filesystem.h"

int main(){
    LocalFileSystem *fs=LocalFileSystem::create();
    FILE *fp;
    int ret = 0;
    ret = fs->fsopen(&fp, "file/origin.txt", "r");
    if(ret < 0){
        printf("open file failed...\n");
        exit(0);
    }
    char buf[1024];
    //fs->fsread(buf,1,10,fd);
    //fs->fsseek(fp,30,SEEK_SET);
    int count = 0;
    while(feof(fp)==0){
        ret = fread(buf, 1, 1, fp);
        if(buf[0]=='\n') printf(" %d", count);
        printf("%c",buf[0]);
        count++;
    }
    return 0;
}
*/