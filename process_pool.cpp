#include"process_pool.h"
#include <iostream>

process_pool* process_pool::_instance = NULL;

extern LocalFileSystem *fs;
extern Mapper *Mapper_Pointer;
extern Reducer *Reducer_Pointer;

static int sig_pipefd[2];

static void addfd(int epollfd, int fd){
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
}

static void removefd(int epollfd, int fd){
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}


process_pool::process_pool(int process_number):process_number(process_number), index(-1), stop(false){
    mm = (mt*)mmap(NULL,sizeof(*mm),PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANON,-1,0);
    memset(mm, 0x00, sizeof(mt));
    pthread_mutexattr_init(&mm->mutexattr);
    pthread_mutexattr_setpshared(&mm->mutexattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mm->mutex,&mm->mutexattr); 
    
    assert((process_number > 0)&&(process_number<=MAX_PROCESS_NUM));
    sub_process = new process[process_number];

    for(int i=0;i<process_number;i++){
        int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sub_process[i].pipefd);
        assert(ret != -1);
        sub_process[i].pid = fork();
        assert(sub_process[i].pid >= 0);
        if(sub_process[i].pid>0){               //master进程
            close(sub_process[i].pipefd[1]);
            continue;
        }
        else{                                   //worker进程
            close(sub_process[i].pipefd[0]);
            index = i;
            break;
        }
    }
}

static void sig_handler(int sig){
    int save_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

static void addsig(int sig, void(handler)(int), bool restart = true){
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if(restart){
        sa.sa_flags = SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL)!=-1);
}


void process_pool::setup_sig_pipe(){
    epollfd = epoll_create(MAX_EVENT_NUM);
    assert(epollfd != -1);
    int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    assert(ret != -1);
    addfd(epollfd, sig_pipefd[0]);

    addsig(SIGINT, sig_handler);
    addsig(SIGCHLD, sig_handler);
}

void process_pool::run(){
    index == -1 ? run_master() : run_worker();
}

void process_pool::run_worker(){
    setup_sig_pipe();

    int pipefd = sub_process[index].pipefd[1];
    addfd(epollfd, pipefd);

    epoll_event events[MAX_EVENT_NUM];
    int ret = -1;
    while(!stop){
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
        if((num<0)&&(errno!=EINTR)){
            printf("epoll failure\n");
            break;
        }
        for(int i=0;i<num;i++){
            int sockfd = events[i].data.fd;
            if((sockfd==pipefd)&&(events[i].events&EPOLLIN)){                   //主进程发来数据
                package_header header;
                ret = recv(sockfd, (void*)&header, sizeof(header), MSG_PEEK);
                if((ret<0)&&(errno!=EAGAIN)||ret==0) continue;
                if(header.package_type == MAP_TASK){
                    map_task_package mtp;
                    ret = recv(sockfd, (void*)&mtp, sizeof(mtp), MSG_DONTWAIT);
                    char buf[1024];
                    FILE *fp;
                    fs->fsopen(&fp, "file/origin.txt", "r");
                    fs->fsseek(fp,mtp.start,SEEK_SET);
                    ret = fs->fsread(buf,1,mtp.end-mtp.start,fp);
                    fs->fsclose(fp);
                    std::string str(buf);

                    Mapper_Pointer->Map(str);
                    Mapper_Pointer->save_map_result(mtp.savefile);

                    map_result_package mrp(SUCCESS);
                    ret = send(sockfd, (void*)&mrp, sizeof(mrp), MSG_DONTWAIT);
                }
                else if(header.package_type == PART_TASK){
                    /*
                    partition_task_package ptp;
                    ret = recv(sockfd, (void*)&ptp, sizeof(ptp), MSG_DONTWAIT);
                    
                    pthread_mutex_lock(&mm->mutex);
                    Mapper_Point->Partition(ptp.savefile);
                    pthread_mutex_unlock(&mm->mutex);

                    partition_result_package prp(SUCCESS);
                    ret = send(sockfd, (void*)&prp, sizeof(prp), MSG_DONTWAIT);
                    */
                }
                else if(header.package_type == SORT_TASK){
                    printf("No.%d worker to do sort task...\n", index);
                    sort_task_package stp;
                    ret = recv(sockfd, (void*)&stp, sizeof(stp), MSG_DONTWAIT);

                    std::cout<<stp.prefix<<std::endl;
                    Mapper_Pointer->Sort(stp.prefix, stp.nfile, stp.savefile);

                    sort_result_package srp(SUCCESS);
                    ret = send(sockfd, (void*)&srp, sizeof(srp), MSG_DONTWAIT);
                }
                else if(header.package_type == REDUCE_TASK){
                    printf("No.%d worker to do reduce task...\n", index);
                    reduce_task_package rtp;
                    ret = recv(sockfd, (void*)&rtp, sizeof(rtp), MSG_DONTWAIT);

                    Reducer_Pointer->set_config(rtp.outputfile);
                    Reducer_Pointer->Combine(rtp.inputfile);
                    //Reducer_Pointer->
                    reduce_result_package rrp(SUCCESS);
                    ret = send(sockfd, (void*)&rrp, sizeof(rrp), MSG_DONTWAIT);
                }
                else if(header.package_type == QUIT_MESS){
                    quit_message_package qmp;
                    ret = recv(sockfd, (void*)&qmp, sizeof(qmp), MSG_DONTWAIT);
                    stop = true;
                    break;
                }
            }
            else if((sockfd==sig_pipefd[0])&&(events[i].events&EPOLLIN)){       //接收信号
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), MSG_DONTWAIT);
                if(ret<=0) continue;
                for(int i=0;i<ret;i++){
                    switch(signals[i]){
                        case SIGINT:
                            stop = true;
                            break;
                        default:
                            break;
                    }
                }
            }
            else{                                                                //其它数据
                continue;
            }
        }
    }

    close(pipefd);
    close(epollfd);
    printf("No.%d process finished..\n", index);
}

void process_pool::run_master(){
    setup_sig_pipe();
    epoll_event events[MAX_EVENT_NUM];

    for(int i=0;i<process_number;i++){
        addfd(epollfd, sub_process[i].pipefd[0]);
    }
    int ret;
    int item_num=0;
    FILE *fp;
    fs->fsopen(&fp, "file/origin.txt", "r");
    assert((fp!=NULL));
    int start=0,end=0;
    while(feof(fp)==0){
        char buf[2];
        ret = fs->fsread(buf,1,1,fp);
        if(buf[0]=='\n'){
            char filename[128];
            sprintf(filename, "file/intermediate%d.txt", item_num);
            map_task_package mtp(start, end, filename);
            if(end-start==0)break;
            start = end+1;
            
            ret = send(sub_process[item_num].pipefd[0], (void*)&mtp, sizeof(mtp), MSG_DONTWAIT);
            if((ret<0)&&(errno!=EAGAIN)){
                printf("master send failure.\n");
            }
            item_num++;
        }
        end++;
    }
    fs->fsclose(fp);
    int count = item_num;
        
    while(!stop){
        int num = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
        if((num<0)&&(errno!=EINTR)){
            printf("epoll failure\n");
            break;
        }
        for(int i=0;i<num;i++){
            int sockfd = events[i].data.fd;
            if((sockfd==sig_pipefd[0])&&(events[i].events&EPOLLIN)){
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), MSG_DONTWAIT);
                if(ret<=0) continue;
                for(int j=0;j<ret;j++){
                    switch(signals[j]){
                        case SIGINT:
                            stop = true;
                            break;
                        case SIGCHLD:
                            pid_t pid;
                            int stat;
                            while((pid=waitpid(-1, &stat, WNOHANG))>0){
                                for(int k=0;k<process_number;k++){
                                    if(sub_process[k].pid==pid){
                                        printf("process %d join...\n",k);
                                        close(sub_process[k].pipefd[0]);
                                        sub_process[k].pid=-1;
                                    }
                                }
                            }
                            stop = true;
                            for(int k=0;k<process_number;k++){
                                if(sub_process[k].pid!=-1){
                                    stop = false;
                                    break;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
            else{
                package_header header;
                ret = recv(sockfd, (void*)&header, sizeof(header), MSG_PEEK);
                if((ret<0)&&(errno!=EINTR)){
                    continue;
                }
                if(ret==0){         //接收到0，说明对面已经关闭了socket
                    continue;
                }
                if(header.package_type == MAP_RES){
                    map_result_package mrp;
                    ret = recv(sockfd, (void*)&mrp, sizeof(mrp), MSG_DONTWAIT);
                    count--;
                    if(count == 0){
                        sort_task_package stp(item_num, "file/intermediate", "file/sort.txt");
                        ret = send(sockfd, (void*)&stp, sizeof(stp), MSG_DONTWAIT);
                    }
                    /*
                    partition_task_package ptp("file/region.txt");
                    ret = send(sockfd, (void*)&ptp, sizeof(ptp), MSG_DONTWAIT);
                    */
                }
                else if(header.package_type == PART_RES){
                    /*
                    partition_result_package prp;
                    ret = recv(sockfd, (void*)&prp, sizeof(prp), MSG_DONTWAIT);
                    
                    item_num--;
                    if(item_num == 0){
                        reduce_task_package rtp("file/region.txt", "file/result.txt");
                        ret = send(sockfd, (void*)&rtp, sizeof(rtp), MSG_DONTWAIT);
                    }
                    */
                }
                else if(header.package_type == SORT_RES){
                    sort_result_package srp;
                    ret = recv(sockfd, (void*)&srp, sizeof(srp), MSG_DONTWAIT);

                    reduce_task_package rtp("file/sort.txt", "file/result.txt");
                    ret = send(sockfd, (void*)&rtp, sizeof(rtp), MSG_DONTWAIT);
                }
                else if(header.package_type == REDUCE_RES){
                    printf("MapReduce is finished successful. The result file is file/result.txt\n");
                    quit_message_package qmp(0);
                    for(int k=0;k<process_number;k++){
                        ret = send(sub_process[k].pipefd[0], (void*)&qmp, sizeof(qmp), MSG_DONTWAIT);
                    }
                }
            }
        }

    }

    close(epollfd);
    printf("master finished...\n");
}