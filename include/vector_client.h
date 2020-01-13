/******************
 * Created by Apollos on 2020/1/8.
 *****************/

#ifndef GAMESERVER_VECTOR_CLIENT_H
#define GAMESERVER_VECTOR_CLIENT_H
typedef struct {
    int fd_normal;      //普通数据收发套接字描述符
    int fd_vital;       //重要数据收发套接字描述符
    int id;             //客户端id
    int blood_value;    //血量
    int isDead;         //是否死亡
}Client;

typedef struct {
    Client *clients;                //套接字描述符，用指针表示
    int counter;            //动态数组里fd的总数
    int max_counter;
}VectorClient;

extern VectorClient* create_vector_client(void);
extern void destroy_vector_client(VectorClient *);
extern Client* get_client(VectorClient *,int index);    //根据索引获得某一个套接字描述符
extern void remove_client(VectorClient *,Client *t);
extern void add_client(VectorClient *,Client);

#endif //GAMESERVER_VECTOR_CLIENT_H
