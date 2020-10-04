#include"process_pool.h"


int main(){
    process_pool* pool=process_pool::create(10);
    pool->run();
    return 0;
}