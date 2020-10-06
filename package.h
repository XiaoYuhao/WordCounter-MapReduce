#include <cstdlib>
#include <cstring>

const u_int8_t MAP_TASK     = 0x01;
const u_int8_t MAP_RES      = 0x11;
const u_int8_t PART_TASK    = 0x02;
const u_int8_t PART_RES     = 0x12;
const u_int8_t SORT_TASK    = 0x03;
const u_int8_t SORT_RES     = 0x13;
const u_int8_t REDUCE_TASK  = 0x04;
const u_int8_t REDUCE_RES   = 0x14;

const u_int8_t QUIT_MESS    = 0xff;

const u_int8_t SUCCESS      = 0x01;
const u_int8_t FAIL         = 0x00;

struct package_header{
    u_int8_t package_type;
    u_int8_t package_seq;
    u_int16_t package_length;
};

struct map_task_package{
    package_header header;
    u_int32_t start;
    u_int32_t end;
    char savefile[128]{0};
    map_task_package(){}
    map_task_package(u_int32_t s, u_int32_t e, const char *filename){
        header.package_type = MAP_TASK;
        header.package_seq = 0;
        header.package_length = sizeof(map_task_package);
        start = s;
        end = e;
        strcpy(savefile, filename);
    }
};


struct map_result_package{
    package_header header;
    u_int8_t result;
    map_result_package(){}
    map_result_package(u_int8_t res){
        header.package_type = MAP_RES;
        header.package_seq = 0;
        header.package_length = sizeof(map_result_package);
        result = res;
    }
};

struct partition_task_package{
    package_header header;
    char savefile[128]{0};
    partition_task_package(){}
    partition_task_package(const char* filename){
        header.package_type = PART_TASK;
        header.package_seq = 0;
        header.package_length = sizeof(partition_task_package);
        strcpy(savefile, filename);
    }
};

struct partition_result_package{
    package_header header;
    u_int8_t result;
    partition_result_package(){}
    partition_result_package(u_int8_t res){
        header.package_type = PART_RES;
        header.package_seq = 0;
        header.package_length = sizeof(partition_result_package);
        result = res;
    }
};

struct sort_task_package{
    package_header header;
    u_int32_t nfile;
    char prefix[128];
    char savefile[128];
    sort_task_package(){}
    sort_task_package(u_int32_t _nfile, const char *_prefix, const char *_savefile){
        header.package_type = SORT_TASK;
        header.package_seq = 0;
        header.package_length = sizeof(sort_task_package);
        nfile = _nfile;
        strcpy(prefix, _prefix);
        strcpy(savefile, _savefile);
    }
};

struct sort_result_package{
    package_header header;
    u_int8_t result;
    sort_result_package(){}
    sort_result_package(u_int8_t res){
        header.package_type = SORT_RES;
        header.package_seq = 0;
        header.package_length = sizeof(sort_result_package);
        result = res;
    }
};


struct reduce_task_package{
    package_header header;
    char inputfile[128]{0};
    char outputfile[128]{0};
    reduce_task_package(){}
    reduce_task_package(const char* ifile, const char* ofile){
        header.package_type = REDUCE_TASK;
        header.package_seq = 0;
        header.package_length = sizeof(reduce_task_package);
        strcpy(inputfile, ifile);
        strcpy(outputfile, ofile);
    }
};

struct reduce_result_package{
    package_header header;
    u_int8_t result;
    reduce_result_package(){}
    reduce_result_package(u_int8_t res){
        header.package_type = REDUCE_RES;
        header.package_seq = 0;
        header.package_length = sizeof(reduce_result_package);
        result = res;
    }
};

struct quit_message_package{
    package_header header;
    quit_message_package(){}
    quit_message_package(u_int8_t n){
        header.package_type = QUIT_MESS;
        header.package_seq = n;
        header.package_length = sizeof(quit_message_package);
    }
};