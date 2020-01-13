/******************
 * Created by Apollos on 2020/1/10.
 *****************/

#ifndef GAMESERVER_SOCKET_UTIL_H
#define GAMESERVER_SOCKET_UTIL_H

extern int readInt(int fd);
extern void writeInt(int fd,int to_write);
extern void writeIntDirectly(int fd,int to_write);
extern void writeIntBySprintf(int fd, int to_write);
extern int* parse_buf(char* buf);
#endif //GAMESERVER_SOCKET_UTIL_H
