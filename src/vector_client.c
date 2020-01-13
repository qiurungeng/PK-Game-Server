/******************
 * Created by Apollos on 2020/1/9.
 *****************/
#include "../include/vector_client.h"
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

static int indexof(VectorClient *vc,Client *client){
    int i=0;
    for (;i<vc->counter;i++){
        if(vc->clients[i].id==client->id){
            return i;
        }
        return -1;
    }
}

static void encapacity(VectorClient *vc){
    if(vc->counter>=vc->max_counter){
        Client *new_clients=(Client*)calloc(vc->counter+5, sizeof(Client));
        assert(new_clients!=NULL);
        memcpy(new_clients,vc->clients, sizeof(Client)*vc->counter);
        free(vc->clients);
        vc->clients=new_clients;
        vc->max_counter+=5;
    }
}

VectorClient* create_vector_client(void){
    VectorClient *vc=(VectorClient*)calloc(1, sizeof(VectorClient));
    assert(vc!=NULL);
    vc->clients=(Client*)calloc(5, sizeof(Client));
    assert(vc->clients!=NULL);
    vc->counter=0;
    vc->max_counter=0;
    return vc;
}
void destroy_vector_client(VectorClient *vc){
    assert(vc!=NULL);
    free(vc->clients);
    free(vc);
}

//获取指定索引的客户端
Client* get_client(VectorClient *vc,int index){
    assert(vc!=NULL);
    if(index<0||index>vc->counter-1)
        return 0;
    return &(vc->clients[index]);
}

void remove_client(VectorClient *vc,Client *client){
    assert(vc!=NULL);
    int index=indexof(vc,client);
    if(index==-1)return;
    int i=index;
    for(;i<vc->counter-1;i++){
        vc->clients[i]=vc->clients[i+1];
    }
    vc->counter--;
}
void add_client(VectorClient *vc,Client client){
    assert(vc!=NULL);
    encapacity(vc);
    vc->clients[vc->counter++]=client;
}

//int main(void){
//    VectorClient* vc=create_vector_client();
//    Client c1,c2,c3;
//    c1.id=1;
//    c1.fd=1;
//    c1.blood_value=100;
//
//    c2.id=2;
//    c2.fd=2;
//    c2.blood_value=100;
//
//    c3.id=3;
//    c3.fd=2;
//    c3.blood_value=100;
//    add_client(vc,c1);
//    add_client(vc,c2);
//
//    printf("%d\n",get_client(vc,0)->id);
//    printf("counter of vc(2):%d\n",vc->counter);
//    add_client(vc,c3);
//    printf("counter of vc(3):%d\n",vc->counter);
//    remove_client(vc,&c1);
//    printf("counter of vc(2):%d\n",vc->counter);
//
//    destroy_vector_client(vc);
//
//    printf("%d\n",(vc->counter));
//    printf("%d\n",(vc!=NULL));
//
//}