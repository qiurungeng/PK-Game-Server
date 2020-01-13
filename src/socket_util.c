/******************
 * Created by Apollos on 2020/1/10.
 *****************/
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/socket_util.h"
#include "../include/vector_client.h"

void int2bytes(int src, unsigned char* message){
    memcpy(message,&src,sizeof(int));
}

int bytes2int(unsigned char* message){
    return message[0]+(message[1]<<8)+(message[2]<<16)+(message[3]<<24);
//    return message[0]+(message[1])+(message[2])+(message[3]);
}

int readInt(int fd){
    char buf[4];
    memset(buf,0, sizeof(buf));
    read(fd,buf, sizeof(buf));
    return atoi((const char *) &buf);
}

void writeInt(int fd,int to_write){
    unsigned char buf[4];
    memset(buf,0, sizeof(buf));
    int2bytes(to_write,buf);
    write(fd, buf, sizeof(int));
}

void writeIntDirectly(int fd,int to_write){
    char buf[4];
    memcpy(buf, &to_write, sizeof(int));
    write(fd,buf, sizeof(int));
}

void writeIntBySprintf(int fd, int to_write){
    char fileSizeStr[4] = {0};
    sprintf(fileSizeStr,"%d",to_write);
//    puts(fileSizeStr);
    write(fd,fileSizeStr, sizeof(int));
}

int* parse_buf(char* buf){
    int *result=(int*)calloc(4, sizeof(int));
    char *ptr,*retptr;
    int i=0;

    ptr = buf;
    while ((retptr=strtok(ptr, "|")) != NULL) {
        result[i]=atoi(retptr);
        i+=1;
        ptr = NULL;
    }

    return result;
}

void send_to_all_clients(VectorClient* vc,char* buf){
    int i=0;
    for (; i < vc->counter; ++i) {
        write(get_client(vc,i)->fd_vital,buf, sizeof(buf));
    }
}