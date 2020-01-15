/******************
 * Created by Apollos on 2020/1/7.
 *****************/
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "../include/vector_client.h"
#include "../include/socket_util.h"
#include "../include/attack.h"

#define TCP_SERVER_PORT 4399
#define HERO_ATTACK_MEG 4
#define HERO_BE_ATTACKED_MSG 5
#define HERO_BE_HURT_MSG 6
#define HERO_DIE_MSG 7
#define HERO_WIN_MSG 8
#define HERO_QUIT_MSG 9
#define BLOOD_REDUCE 10

//VectorFD *vfd;
VectorClient *vc;
VectorAttack *va;
int sockfd_tcp;
static int CLIENT_ID=100;
static int UDP_SERVER_PORT=2019;
static int ATTACK_EVENT_ID=0;
static int ALIVE_PLAYER_COUNT=0;    //场上存活玩家数

void sig_handler(int signo){
    if(signo==SIGINT){
        printf("server close!\n");
        //关闭socket
        close(sockfd_tcp);
        int i;
        for (i = 0; i <vc->counter; ++i) {
            close(get_client(vc,i)->fd_normal);
            close(get_client(vc,i)->fd_vital);
        }
        //销毁动态数组
        destroy_vector_client(vc);
        destroy_vector_attack(va);
        exit(1);
    }
}


/**
 * 打印连接上的客户端的地址
 * @param clientaddr
 */
void out_addr(struct sockaddr_in *clientaddr){
    char ip[16];
    memset(ip,0, sizeof(ip));
    int port=ntohs(clientaddr->sin_port);
    inet_ntop(AF_INET,&clientaddr->sin_addr.s_addr,ip, sizeof(ip));
    printf("%s(%d) connected!\n",ip,port);
}

int make_UDP_socket(int local_udp_port){
    struct sockaddr_in server_addr;
    int sockfd_udp=socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd_udp < 0){
        perror("socket error");
        exit(1);
    }

    memset(&server_addr,0, sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(local_udp_port);
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(sockfd_udp, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind error");
        exit(1);
    }
    return sockfd_udp;
}

/**
 * 为已通过TCP连接的客户端建立UDP连接，由客户端指定的端口 连接到 服务器指定的端口
 * Server_udp --> Client1_udp,Client2_udp,Client3_udp...ClientN_ucp
 * @param client_addr,server_udp_port,client_udp_port
 */
int make_UDP_connection_for_client(struct sockaddr_in *client_addr, int server_udp_port, int client_udp_port){
    struct sockaddr_in client_addr_udp;

    int fd=make_UDP_socket(server_udp_port);

    //通过建立TCP初始连接得到的信息构建客户端UDP地址
    memset(&client_addr_udp, 0, sizeof(client_addr_udp));
    client_addr_udp.sin_family=AF_INET;
    client_addr_udp.sin_addr.s_addr=client_addr->sin_addr.s_addr;
    client_addr_udp.sin_port=htons(client_udp_port);

    connect(fd, (struct sockaddr*)&client_addr_udp, sizeof(struct sockaddr)); //绑定客户端UDP1地址

    return fd;
}

/**
 * 为客户端玩家创建Client并加入到动态数组中，维护其连接及游戏数据
 * @param client_addr   客户端地址
 * @param tcp_fd        TCP连接套接字
 * @return
 */
Client* create_Client(struct sockaddr_in *client_addr,int tcp_fd){
    //接收客户端发来的UDP端口号
    int client_udp_port=readInt(tcp_fd);
    printf("Client UDP Port:%d\n",client_udp_port);
    //接收客户端发来的UDP端口号2
    int client_udp_port2=readInt(tcp_fd);
    printf("Client UDP Port2:%d\n",client_udp_port2);
    //为客户端分配ID
    int client_id=CLIENT_ID++;
    writeIntBySprintf(tcp_fd, client_id);
    //打印连接成功的客户端的地址
    out_addr(client_addr);

    Client client;
    client.id=client_id;
    client.blood_value=100;
    int udp1=UDP_SERVER_PORT++;
    writeIntBySprintf(tcp_fd, udp1);
    printf("udp1:%d\n",udp1);
    client.fd_normal=make_UDP_connection_for_client(client_addr,udp1,client_udp_port);
    int udp2=UDP_SERVER_PORT++;
    printf("udp2:%d\n",udp2);
    writeIntBySprintf(tcp_fd, udp2);
    client.fd_vital=make_UDP_connection_for_client(client_addr,udp2,client_udp_port2);
    client.isDead=0;
    add_client(vc,client);
    ALIVE_PLAYER_COUNT++;

    printf("\n******NEW CLIENT HAS JOINED******\n");
    printf("CLIENT ID:%d\n",get_client(vc,vc->counter-1)->id);
    printf("CLIENT BLOOD:%d\n",get_client(vc,vc->counter-1)->blood_value);
    printf("now there are %d clients in game.\n",ALIVE_PLAYER_COUNT);
    int i;
    for(i=0;i<vc->counter;i++){
        Client *alive=get_client(vc,i);
        printf("alive client:%d,normal_port:%d,vital_port:%d\n",alive->id,alive->fd_normal,alive->fd_vital);
    }
    printf("*********************************\n\n");

}

/**
 * 遍历动态数组中所有的描述符并加入到描述符集set中
 * 同时返回动态数组中最大的那个描述符
 * @param set
 * @return max_fd
 */

//普通消息转发描述符
int add_set_normal(fd_set *set){
    FD_ZERO(set);   //清空描述符集
    int max_fd=0;
    int i=0;
    for(;i<vc->counter;i++){
        int fd=get_client(vc,i)->fd_normal;
        if(fd>max_fd){
            max_fd=fd;
        }
        FD_SET(fd,set);     //将fd加入到描述符集当中
    }
    return max_fd;
}

//重要消息处理转发描述符
int add_set_vital(fd_set *set){
    FD_ZERO(set);   //清空描述符集
    int max_fd=0;
    int i=0;
    for(;i<vc->counter;i++){
        int fd=get_client(vc,i)->fd_vital;
        if(fd>max_fd){
            max_fd=fd;
        }
        FD_SET(fd,set);     //将fd加入到描述符集当中
    }
    return max_fd;
}

/**
 * 处理普通信息：转发角色动作、位置
 * @param fd：对应于某个连接的客户端
 * 和某一个连接的客户端进行双向通信(非阻塞方式)
 */
void do_service_normal(Client* client){
    /*和客户端进行读写操作(双向通信)*/
//    printf("Do normal service for Client:%d\n",client->id);
    char buff[1024];
    memset(buff,0, sizeof(buff));
    /**
     * 因为采用非阻塞方式读取，读不到数据直接返回
     * 直接服务于下一个客户端
     * 因此不需要判断size<0的情况。
     */
    //读取客户端发来信息
    size_t size=read(client->fd_normal,buff, sizeof(buff));
    if(size==0){    //客户端已经关闭连接
        printf("client closed\n");
        //从动态数组中删除对应的fd
        remove_client(vc,client);
        //关闭对应客户端的socket
        close(client->fd_normal);
    } else if(size>0){
        //将从客户端读取到的信息写回所有在线的客户端
        int i;
        for (i = 0; i < vc->counter; ++i) {
            Client* write_to=get_client(vc,i);
            int n;
            if ((n=write(write_to->fd_normal,buff, size))<0){
                if(errno==EPIPE){   //客户端关闭连接
                    perror("write error");
                    remove_client(vc,client);
                    remove_client(vc,client);
                    close(client->fd_normal);
                }
            }
        }
    }
}


/**
 * 处理重要信息：攻击、死亡
 * @param fd：对应于某个连接的客户端
 * 和某一个连接的客户端进行双向通信(非阻塞方式)
 */
void do_service_vital(Client* client){
    /*和客户端进行读写操作(双向通信)*/
    printf("Do vital service for Client:%d\n",client->id);
    char buff[1024];
    char to_split[1024];
    memset(buff,0, sizeof(buff));

    /**
     * 因为采用非阻塞方式读取，读不到数据直接返回
     * 直接服务于下一个客户端
     * 因此不需要判断size<0的情况。
     */

    //读取客户端发来信息
    size_t size=read(client->fd_vital,buff, sizeof(buff));
    if(size==0){    //客户端已经关闭连接
        printf("client closed\n");
        //从动态数组中删除对应的fd
        remove_client(vc,client);
        //关闭对应客户端的socket
        close(client->fd_vital);
    } else if(size>0){

        memcpy(to_split,buff, sizeof(buff));

        int atk_id,atk_x,atk_y,atk_ftr,eventId,be_atk_x,be_atk_y;
        int i,j;
        char* pSave=NULL;
        char* pSave2=NULL;
        int msg=atoi(strtok_r(to_split,"|",&pSave));
        printf("service msg type:%d\n",msg);
        switch (msg){
            case HERO_ATTACK_MEG:
                //解析Java传来的攻击消息数据
                atk_id=atoi(strtok_r(NULL,"|",&pSave));
                atk_x=atoi(strtok_r(NULL,"|",&pSave));
                atk_y=atoi(strtok_r(NULL,"|",&pSave));
                atk_ftr=atoi(strtok_r(NULL,"|",&pSave));
                for ( i = 0; i < vc->counter; i++) {
                    Client *opponent=get_client(vc,i);
                    if(atk_id==opponent->id){
                        continue;
                    }
                    Attack *attack=create_attackMsg(ATTACK_EVENT_ID++,atk_id,opponent->id,atk_x,atk_y,atk_ftr);
                    add_attack(va,*attack);
                    char buf[sizeof(int)*2+ sizeof('|')*2];
                    sprintf(buf,"%d|%d|",HERO_BE_ATTACKED_MSG,attack->eventId);
                    write(opponent->fd_vital,buf, sizeof(buf));
                }
                break;
            case HERO_BE_ATTACKED_MSG:
                //解析Java传来的被打消息数据
                strtok_r(buff,"|",&pSave2);
                eventId=atoi(strtok_r(NULL,"|",&pSave2));
                be_atk_x=atoi(strtok_r(NULL,"|",&pSave2));
                be_atk_y=atoi(strtok_r(NULL,"|",&pSave2));

                //得到被攻击者坐标后，处理攻击事件
                for (i = 0; i < va->counter; i++) {
                    Attack *beAtk=get_attack(va,i);

                    if(beAtk->eventId==eventId){
                        //被攻击者坐标
                        beAtk->beAttacked_x=be_atk_x;
                        beAtk->beAttacked_y=be_atk_y;

                        int diff_x=beAtk->attack_x-beAtk->beAttacked_x;
                        int diff_y=beAtk->attack_y-beAtk->beAttacked_y;
                        //判断是否击中
                        if ((-10<=diff_y&&diff_y<=10)&&((0<=diff_x&&diff_x<=60&&!beAtk->faceToRight)||(-60<=diff_x&&diff_x<0&&beAtk->faceToRight))) {

                            //被击中英雄受伤消息,死亡消息
                            char beHurtMsg[(sizeof(int)+ sizeof("|"))*3];
                            char dieMsg[(sizeof(int)+ sizeof("|"))*2];
                            sprintf(beHurtMsg,"%d|%d|%d|",HERO_BE_HURT_MSG,beAtk->beAttackedPlayerId,BLOOD_REDUCE);
                            //死亡标志
                            int deadFlag=0;
                            //服务器端记录英雄扣血
                            for(j=0;j<vc->counter;j++){
                                Client *c=get_client(vc,j);
                                if (c->id==beAtk->beAttackedPlayerId){

                                    //忽略已死英雄
                                    if (c->isDead!=0){
                                        return;
                                    }

                                    c->blood_value-=BLOOD_REDUCE;
                                    printf("被打英雄:%d,还剩血量:%d\n",c->id,c->blood_value);
                                    if(c->blood_value<=0){
                                        printf("被打英雄:%d,死亡\n",c->id);
                                        deadFlag=1;
                                        c->isDead=1;    //标记为将死
                                        sprintf(dieMsg,"%d|%d|",HERO_DIE_MSG,beAtk->beAttackedPlayerId);
                                    }
                                    break;
                                }

                            }
                            //群发到客户端
                            for(j=0;j<vc->counter;j++){
                                Client *c=get_client(vc,j);
                                if(deadFlag){
                                    //群发死亡通知
                                    write(c->fd_vital,dieMsg, sizeof(dieMsg));
                                    //更新服务端数据
                                    if (c->isDead==1){
                                        c->isDead=2;                //标记为已死
                                        ALIVE_PLAYER_COUNT--;       //场上存活玩家数量更新
                                    }
                                }
                                //群发受伤通知
                                write(c->fd_vital,beHurtMsg, sizeof(beHurtMsg));
                            }
                            //判断是否满足胜利条件
                            if (ALIVE_PLAYER_COUNT==1){
                                int winnerId=0;
                                for (j = 0; j < vc->counter; ++j) {
                                    Client *c=get_client(vc,j);
                                    if (c->isDead==0){
                                        winnerId=c->id;
                                    }
                                }
                                char winMsg[(sizeof(int)+ sizeof("|"))*2];
                                sprintf(winMsg,"%d|%d|",HERO_WIN_MSG,winnerId);
                                for (j = 0; j < vc->counter; ++j) {
                                    Client *c=get_client(vc,j);
                                    write(c->fd_vital,winMsg, sizeof(winMsg));
                                }
                            }
                        }
                    }
                }

                break;
            case HERO_QUIT_MSG:
                printf("Client:%d has quited!\n",client->id);

                char buf[(sizeof(int)+ sizeof("|"))*2];
                sprintf(buf,"%d|%d|",HERO_QUIT_MSG,client->id);
                for (i = 0; i<vc->counter ; i++) {
                    Client *to_notify=get_client(vc,i);
                    if (to_notify->id==client->id)continue;
                    printf("notify client:%d about this QUIT msg\n",to_notify->id);
                    write(to_notify->fd_vital,buf, sizeof(buf));
                }
                ALIVE_PLAYER_COUNT--;
                close(client->fd_vital);
                close(client->fd_normal);
                remove_client(vc,client);
                printf("Now there are %d client in Game.\n",vc->counter);
                for(i=0;i<vc->counter;i++){
                    Client *alive=get_client(vc,i);
                    printf("alive client:%d,normal_port:%d,vital_port:%d\n",alive->id,alive->fd_normal,alive->fd_vital);
                }
            default:
                break;
        }
        pSave=NULL;
        pSave=NULL;
    }
}


/**
 * 转发消息线程，Java服务端自己发送自己解析
 * @param arg
 * @return
 */
void* th_fn_normal(void *arg){
    struct timeval t;
    t.tv_sec=2;     //超时时间：秒
    t.tv_usec=0;    //超时时间：微秒
    int n;
    int maxfd;      //最大描述符
    fd_set set_normal;     //描述符集
    maxfd= add_set_normal(&set_normal);

    /* 调用select函数会阻塞，委托内核去检查传入的描述符是否准备好
     * 若有则返回准备好的描述符个数,超时则返回0
     * 第一个参数：描述符集中描述符的范围（最大描述符+1）
     */
    while ((n=select(maxfd+1, &set_normal, NULL, NULL, &t)) >= 0){
        if(n>0){
            /*
             * 检测哪些描述符准备好
             * 并和这些准备好描述符集对应的客户端进行双向通讯
             */
            int i=0;
            for(;i<vc->counter;i++){
                Client* client=get_client(vc,i);
                int fd=client->fd_normal;
                if(FD_ISSET(fd,&set_normal)){
                    do_service_normal(client);
                }
            }
        }
        //重新设置时间和清空描述符集
        t.tv_sec=2;
        t.tv_usec=0;
        //重新遍历动态数组中最新的描述符，放置到描述符集中
        maxfd= add_set_normal(&set_normal);
    }
    return (void*)0;
}

/**
 * 重要消息处理转发线程，打开解析后处理后发送反馈
 * Java的常用Socket编程读写方法适用于C，
 * 两端先将传输数据转成字符串，用指定分隔符分隔后传输，收到后进行字符串字节流解析。
 * @param arg
 * @return
 */
void* th_fn_vital(void *arg){
    struct timeval t;
    t.tv_sec=2;     //超时时间：秒
    t.tv_usec=0;    //超时时间：微秒
    int n;
    int maxfd;      //最大描述符
    fd_set set_vital;     //描述符集
    maxfd= add_set_vital(&set_vital);

    /* 调用select函数会阻塞，委托内核去检查传入的描述符是否准备好
     * 若有则返回准备好的描述符个数
     * 超时则返回0
     * 第一个参数：描述符集中描述符的范围（最大描述符+1）
     */
    while ((n=select(maxfd+1, &set_vital, NULL, NULL, &t)) >= 0){
        if(n>0){
            /* 检测哪些描述符准备好
             * 并和这些准备好描述符集对应的客户端进行双向通讯
             */
            int i=0;
            for(;i<vc->counter;i++){
                Client* client=get_client(vc,i);
                int fd=client->fd_vital;
                if(FD_ISSET(fd,&set_vital)){
                    do_service_vital(client);
                }
            }
        }
        //重新设置时间和清空描述符集
        t.tv_sec=2;
        t.tv_usec=0;
        //重新遍历动态数组中最新的描述符，放置到描述符集中
        maxfd= add_set_vital(&set_vital);
    }
    return (void*)0;
}

/**
 * 程序入口
 * 编译运行方式:项目路径下,输入：
 * (cc -o bin/game_server src/vector_client.c src/socket_util.c src/attack.c src/game_server.c -lpthread)
 * @param argc
 * @param argv：传入服务器TCP端口
 * @return
 */
int main(int argc,char *argv[]){
    if(argc<2){
        printf("usage: %s #port\n",argv[0]);
        exit(1);
    }
    if(signal(SIGINT,sig_handler)==SIG_ERR){
        perror("signal sigint error");
        exit(1);
    }

    //TCP Socket 创建
    sockfd_tcp=socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd_tcp < 0){
        perror("socket error");
        exit(1);
    }

    //调用bind函数将socket和地址(ip,addr)进行绑定
    struct sockaddr_in serveraddr;
    memset(&serveraddr,0, sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(atoi(argv[1]));
    serveraddr.sin_addr.s_addr=INADDR_ANY;
    if(bind(sockfd_tcp, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
        perror("bind error");
        exit(1);
    }

    /* 调用listen函数监听指定port
     * 通知系统去接收来自客户端的连接请求
     * 将接收到的客户端连接放置在对应的队列中
     * 第二个参数：指定队列的长度
     */
    if(listen(sockfd_tcp, 10) < 0){
        perror("listen error");
        exit(1);
    }

    //创建客户端的动态数组
    vc=create_vector_client();
    //创建攻击事件动态数组
    va=create_vector_attack();

    /**
      * 1）主控线程获得客户端的连接
      * 2）由TCP读写与服务端交换必要数据，为服务端创建新的Client并放到动态数组当中
      * 3）每个Client保存有客户端普通消息、重要消息转发的两个socket描述符
      * 4）分离状态启动两个子线程，分别负责游戏普通消息和重要消息处理
      * 5.1）启动的子线程调用select函数委托内核检查传入到select中的描述符集是否准备好
      * 5.2）利用FD_ISSET来找出那些已经准备好的描述符，并与对应的客户端进行双向通信
      *      (非阻塞方式读写)
      */

    //分离状态启动NORMAL数据转发线程
    pthread_t th;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    int err;
    //创建普通消息转发线程
    if((err=pthread_create(&th,&attr,th_fn_normal,(void*)0))!=0){
        perror("normal data handling thread create error");
        exit(1);
    }
    //创建重要消息处理线程
    if((err=pthread_create(&th,&attr,th_fn_vital,(void*)0))!=0){
        perror("vital data handling thread create error");
        exit(1);
    }
    pthread_attr_destroy(&attr);



    /**
     * TCP连接，用于获取客户端地址及其发来的UDP端口
     * 为其创建Client，维护其相关的连接及游戏数据，保存在客户端动态数组中
     */
    struct sockaddr_in clientaddr;
    socklen_t len= sizeof(clientaddr);
    while (1){
        memset(&clientaddr,0,len);
        //调用accept函数从队列中获得一个客户端的请求连接，并返回新的socket描述符
        int fd=accept(sockfd_tcp, (struct sockaddr*)&clientaddr, &len);
        if(fd<0){
            perror("accept error");
            continue;
        }
        //为该客户端创建Client结构体,用来维护管理其UDP连接与游戏数据
        create_Client(&clientaddr,fd);
        //断开TCP连接
        close(fd);
    }


}
