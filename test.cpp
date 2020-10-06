
#include"process_pool.h"

Mapper *Mapper_Pointer = NULL;
Reducer *Reducer_Pointer = NULL;

bool isspace(char c){
    if(c==' ')return true;
    else return false;
}

class WordCounter:public Mapper
{
public:
    virtual void Map(const std::string &input){
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

class Adder:public Reducer
{
public:
    virtual void Reduce(const std::string &key, const std::vector<std::string> &valist){
        u_int64_t value = 0;
        for(auto val: valist){
            value += stoi(val);
        }
        Emit(key, std::to_string(value));
    }    
};


int main(){
    WordCounter obj;
    Mapper_Pointer = &obj;
    Adder reduce_obj;
    Reducer_Pointer = &reduce_obj;

    process_pool* pool=process_pool::create(10);
    pool->run();
    return 0;
}
