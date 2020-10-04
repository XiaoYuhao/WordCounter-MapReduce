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
#include <fcntl.h>  
#include <stdlib.h>  
#include <sys/epoll.h>  
#include <signal.h>  
#include <sys/wait.h>  
#include <sys/stat.h>

struct process{
    pid_t pid;
    int pipefd[2];
};

class process_pool{
private:
    process_pool(int process_number=10);
    static process_pool* _instance;
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