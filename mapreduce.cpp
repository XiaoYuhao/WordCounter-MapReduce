#include"mapreduce.h"

LocalFileSystem *fs=LocalFileSystem::create();

int _readline(FILE *_fp, std::string &_key, std::string &_val){
    char _key_buf[1024];
    char _val_buf[1024];
    if(fs->fsscanf(_fp, "%s %s", _key_buf, _val_buf) != EOF){
        _key = _key_buf;
        _val = _val_buf;
        return 1;
    }
    else return 0;
}

int _writeline(FILE *_fp, const std::string &_key, const std::string &_val){
    return fs->fsprintf(_fp, "%s %s\n", _key.c_str(), _val.c_str());
}

Mapper::Mapper(){

}

Mapper::~Mapper(){

}
void Mapper::Map(const std::string &input){
    printf("Base Mapper object.\n");
}

void Mapper::Emit(const std::string &key, const std::string &val){
    /*
    fs->fswrite(key.c_str(), key.length(), 1, fp);
    fs->fswrite(" ", 1, 1, fp);
    fs->fswrite(value.c_str(), value.length(), 1, fp);
    fs->fswrite("\n", 1, 1, fp);
    */
   //fs->fsprintf(fp, "%s %s\n", key.c_str(), val.c_str());
   if(mmap.count(key)){
       mmap[key]++;
   }
   else{
       mmap[key]=1;
   }
}

void Mapper::Partition(const char *filename){
    /*
    char buf[4096];
    fs->fsseek(fp, 0, SEEK_SET);
    fs->fsread(buf, sizeof(buf), 1, fp);
    FILE *ffp;
    fs->fsopen(&ffp, filename, "a+");
    fs->fswrite(buf, strlen(buf), 1, ffp);
    fs->fsclose(ffp);
    */
} 

void Mapper::Sort(const char *prefix, int nfile, const char* savefile){
    struct node{
        std::string key;
        std::string val;
        int flag;
        bool operator > (const node &a) const {
            return key > a.key;
        }
    };
    std::priority_queue<node, std::vector<node>, std::greater<node>> q;
    char filename[128];
    FILE **ffp = (FILE**)malloc(nfile*sizeof(FILE*));
    for(int i=0;i<nfile;i++){
        sprintf(filename, "%s%d.txt", prefix, i);
        fs->fsopen(&ffp[i], filename, "r");
    }

    FILE *out;
    fs->fsopen(&out, savefile, "a+");
    int ret = 0;
    std::string key,val;
    node tmp;
    for(int i=0;i<nfile;i++){
        tmp.flag = i;
        ret = _readline(ffp[i], tmp.key, tmp.val);
        if(ret == 0) continue;
        q.push(tmp);
    }
    while(!q.empty()){
        tmp = q.top();
        q.pop();
        //printf("key: %s val: %s\n", tmp.key.c_str(), tmp.val.c_str());
        _writeline(out, tmp.key, tmp.val);
        if(_readline(ffp[tmp.flag], tmp.key, tmp.val)){
            q.push(tmp);
        }
    }
    fs->fsclose(out);
    for(int i=0;i<nfile;i++){
        fs->fsclose(ffp[i]);
    }
    free(ffp);
}


void Mapper::save_map_result(const char *filename){
    FILE *fp;
    fs->fsopen(&fp, filename, "a+");
    for(auto &item: mmap){
        fs->fsprintf(fp, "%s %d\n", item.first.c_str(), item.second);
    }
    fs->fsclose(fp);
}

Reducer::Reducer(){

}

Reducer::~Reducer(){

}

void Reducer::Emit(const std::string &key, const std::string &value){
    _writeline(fp, key, value);
}

void Reducer::Reduce(const std::string &key, const std::vector<std::string> &valist){
    printf("Base Reduce Object.\n");
}

void Reducer::Combine(const char *filename){
    std::map<std::string, std::vector<std::string>> mmap;
    FILE* ffp;
    fs->fsopen(&ffp, filename, "r");
    int ret = 0;
    std::string key, val;
    while(true){
        if(_readline(ffp, key, val)){
            mmap[key].push_back(val);
        }
        else{
            break;
        }
    }
    fs->fsclose(ffp);

    for(auto &item: mmap){
        Reduce(item.first, item.second);
    }

    fs->fsclose(fp);

}

void Reducer::set_config(const char *filename){
    fs->fsopen(&fp, filename, "a+");
}