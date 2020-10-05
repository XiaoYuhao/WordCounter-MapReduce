#ifndef _PROCESS_POOL_H
#define _PROCESS_POOL_H

#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <assert.h>  
#include <stdio.h>  
#include <unistd.h>  
#include <errno.h>  
#include <string.h>  
#include <string>
#include <fcntl.h>  
#include <stdlib.h>  
#include <sys/epoll.h>  
#include <signal.h>  
#include <sys/wait.h>  
#include <sys/stat.h>
#include <sys/mman.h>
#include <pthread.h>
#include "filesystem.h"
#include "mapreduce.h"
#include "package.h"

struct process{
    pid_t pid;
    int pipefd[2];
};

struct mt
{
    int num;
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexattr;
};

class process_pool{
private:
    process_pool(int process_number=10);
    ~process_pool();
    static process_pool* _instance;
    struct mt *mm;
public:
    static process_pool* create(int process_number=10){
        if(_instance==NULL){
            _instance = new process_pool(process_number);
        }
        return _instance;
    }
    void run();
private:
    void setup_sig_pipe();
    void run_master();
    void run_worker();
private:
    static const int MAX_PROCESS_NUM = 16;
    static const int MAX_EVENT_NUM = 10000;
    int process_number;
    int index;
    int epollfd;
    int listen_fd;
    int stop;
    process* sub_process;
};

#endif