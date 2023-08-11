#include <iostream>
#include "libtcp.h"
#include "loger.h"
#include "thread_pool.h"
#include "serverx.h"

using namespace std;
char* program_name;
#define THREAD_POOL_SIZE    4
const char* pNameFile;
FILE *flog;
const char* port_out = "9050";        //порт out
const char* port_in = "9000";        //порт in
const char* device1 = "enp3s0";          //карта 1 - сеть 1
const char* device2 = "enp4s0";          //карта 2 - сеть 2
const char* ip_addr2 = "192.168.1.50";      //адрес первый
const char* ip_addr1 = "192.168.0.65";      //адрес второй

std::mutex mtxPrintF;
std::mutex coutMutex;


int main(int argc, char *argv[])
{
    setlocale(LC_ALL,"ru");
    INIT();
    pNameFile = "Server.log";

    ThreadPool pool(THREAD_POOL_SIZE);
    loger(INFO, 0, "Инициирован пул потоков размером %d", THREAD_POOL_SIZE);
    SafeCout scout;
    pool.add_task([&](){
        ServerXOI S1(ip_addr1,port_out,port_in,device1,&scout);});
    pool.add_task([&](){
        ServerXOI S2(ip_addr2,port_out,port_in,device2,&scout);});
    

    return 0;
}
