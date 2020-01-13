cmake_minimum_required(VERSION 3.14)
project(MyGameServer C)

set(CMAKE_C_STANDARD 99)

add_executable(MyGameServer main.c include/attack.h include/socket_util.h include/vector_client.h src/game_server.c src/attack.c src/socket_util.c src/vector_client.c)