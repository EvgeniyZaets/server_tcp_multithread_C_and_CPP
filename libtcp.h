#pragma once
#ifndef LIBTCP_H
#define LIBTCP_H

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "skel.h"
#define NLISTEN		5		        //максимальное колличество подключений

extern char* program_name;		    //имя программы для логирования и ошибок

//вывод и обработка ошибок
void error(int, int, const char*, ...);

//считывание данных ровно n байт
u_int32_t readn(SOCKET, char*, u_int32_t);

//считываение данных переменной длины
u_int32_t readvrec( SOCKET, char *, u_int32_t );

//фукнция заполнение структуры адреса
void set_address(const char*,const char*, struct sockaddr_in*, char*);

//TCP
int tcp_client(const char*,const  char*);                           //Клиент TCP
int tcp_server(const char*,const  char*);                           //Сервер TCP
int tcp_server(const char* hname,const  char* sname,const char* device); //Сервер TCP на конкретную сетевую карту

//UDP
int udp_client(const char*,const  char*, struct sockaddr_in*);          //Клиент UDP
int udp_server(const char* ,const  char* );                             //Сервер UDP
int udp_server(const char* hname,const char* sname,const char* device);      //Сервер UDP на конкретную сетевую карту

#endif // LIBTCP_H
