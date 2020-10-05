#include"process_pool.h"


process_pool* process_pool::_instance = NULL;

extern LocalFileSystem *fs;
extern Mapper *Mapper_Point;
extern Reducer *Reducer_Point;

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
                /*
                char recvbuf[1024];
                ret = recv(sockfd, recvbuf, sizeof(recvbuf), MSG_DONTWAIT);
                if((ret<0)&&(errno!=EAGAIN)||ret==0) continue;
                if(recvbuf[0]=='q'){
                    stop = true;
                    break;
                }
                else printf(recvbuf);
                char sendbuf[1024];
                sprintf(sendbuf, "This is No.%d process...\n", index);
                ret = send(sockfd, sendbuf, strlen(sendbuf), MSG_DONTWAIT);
                if((ret<0)&&(errno!=EAGAIN)){
                    printf("send failure.\n");
                    continue;
                }*/
                Message mes;
                ret = recv(sockfd, (void*)&mes, sizeof(mes), MSG_DONTWAIT);
                if((ret<0)&&(errno!=EAGAIN)||ret==0) continue;
                char buf[1024];
                FILE *fp;
                fs->fsopen(&fp, "file/origin.txt", "r");
                fs->fsseek(fp,mes.start,SEEK_SET);
                ret = fs->fsread(buf,1,mes.end-mes.start,fp);
                printf(buf);
                fs->fsclose(fp);
                std::string str(buf);

                Mapper_Point->set_config(mes.savefile);
                Mapper_Point->Map(str);

                char sendbuf[1024] = "map function finished...\n";
                ret = send(sockfd, sendbuf, strlen(sendbuf), MSG_DONTWAIT);
                char recvbuf[1024];
                ret = recv(sockfd, recvbuf, sizeof(recvbuf), MSG_DONTWAIT);
                if((ret<0)&&(errno!=EAGAIN)||ret==0) continue;


                stop = true;
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
    int now=0;
    FILE *fp;
    fs->fsopen(&fp, "file/origin.txt", "r");
    assert((fp!=NULL));
    int start=0,end=0;
    while(feof(fp)==0){
        char buf[2];
        ret = fs->fsread(buf,1,1,fp);
        if(buf[0]=='\n'){
            Message mes;
            mes.start = start;
            mes.end = end;
            if(end-start==0)break;
            start = end+1;
            sprintf(mes.savefile, "file/intermediate%d.txt", now);
            ret = send(sub_process[now].pipefd[0], (void*)&mes, sizeof(mes), MSG_DONTWAIT);
            if((ret<0)&&(errno!=EAGAIN)){
                printf("master send failure.\n");
            }
            now++;
        }
        end++;
    }
    fs->fsclose(fp);
        
    /*
    for(int i=0;i<process_number;i++){
        char sendbuf[1024];
        sprintf(sendbuf, "This master send message to No.%d process.\n", i);
        ret = send(sub_process[i].pipefd[0], sendbuf, strlen(sendbuf), MSG_DONTWAIT);
        if((ret<0)&&(errno!=EAGAIN)){
            printf("master send failure.\n");
        }
    }
    */
    
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
                char recvbuf[1024];
                ret = recv(sockfd, recvbuf, sizeof(recvbuf), MSG_DONTWAIT);
                if((ret<0)&&(errno!=EINTR)){
                    continue;
                }
                if(ret==0){         //接收到0，说明对面已经关闭了socket
                    continue;
                }
                printf(recvbuf);
                char sendbuf[1024] = "file/region.txt";
                send(sockfd, sendbuf, strlen(sendbuf)+1, MSG_DONTWAIT);
                if((ret<0)&&(errno!=EAGAIN)){
                    printf("master send failure.\n");
                }
            }
        }

    }

    close(epollfd);
    printf("master finished...\n");
}