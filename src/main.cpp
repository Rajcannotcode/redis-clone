#include "../include/RedisServer.h"
#include "../include/RedisDatabase.h"
#include <iostream>

#include<thread>
#include<chrono>

int main(int argc, char* argv[]){

    int port = 6379; //default

    //stoi converts string to integer
    //we see if any port has been passed
    //in case user wants set a specific port
    if(argc >= 2) port = std::stoi(argv[1]);

    if(RedisDatabase::getInstance().load("dump.my_rdb"))
        std::cout << "Database loaded from dump.my_rdb\n";
    else
        std::cout<< "No dump found or load failed; starting with an empty database\n";

    RedisServer server(port);

    //Background persistance: dump the database every 300 seconds, we save db after every 5 mins
    //the thread here is declared as a lamda expression
    std::thread persistenceThread([](){
        while(true){
            std::this_thread::sleep_for(std::chrono::seconds(300));
            
            //use bgsave so the main server doesn't freeze
            if(!RedisDatabase::getInstance().bgdump("dump.my_rdb")){
                std::cerr << "Error starting background save\n";
            }
            else{
                std::cout<<"Background save initiated...\n";
            }
        }
    });
    persistenceThread.detach();

    server.run();

    return 0;
}