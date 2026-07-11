#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"

#include <iostream>
#include <sys/socket.h>
//gives the functions required to build both the servers and the clients
#include <unistd.h>
//unix standard, provides tools to talk directly to the unix operating system
//used for closing server socket here
#include <netinet/in.h>

#include <vector>
#include <thread>
#include <cstring>
#include <signal.h>

static RedisServer* globalServer = nullptr;
//static in this context means that this variable is private to this specific file i.e. RedisServer.cpp

void signalHandler(int signum){
    if(globalServer){
        std::cout << "Caught Signal " << signum << ", shutting down...\n";
        globalServer->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandler(){
    signal(SIGINT, signalHandler);
    signal(SIGCHLD, SIG_IGN);
}

//RedisServer:: tells the compiler this function belongs in the RedisServer class
//RedisServer(int port) is the constructor itself
//:port(port) intialises the port parameter to the port value and so on
RedisServer::RedisServer(int port): port(port), server_socket(-1), running(true){

    globalServer = this;
    setupSignalHandler();
}


void RedisServer::shutdown(){
    running = false;
    if(server_socket != -1){
        if(RedisDatabase::getInstance().dump("dump.my_rdb")){
            std::cout << "Database dumper to dump.my_rdb \n";
        }
        else{
            std::cerr<< "Error dumping database \n";
        }

        close(server_socket);
    }

    std::cout << "Server Shutdown complete!\n";
}


void RedisServer::run(){
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    //asks the operating system to create a new socket
    //AF_INET address family-internet, this socket will use the IPv4 standard
    //for IPv6 use AF_INET6
    //SOCK_STREAM means we want a TCP connection, TCP has perfect data accuracy
    //if wanted less accuracy we could use UDP which would use SOCK_DGRAM
    //0 asks the OS to choose the default protocol for the combination of AF_INET and SOCK_STREAM

    if(server_socket < 0){
        std::cerr<<"Error creating server socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    //we set socket options here
    //SOL_SOCKET, socket level networking, tells the OS the setting we want to change is in the top level socket itself
    //SO_REUSEADDR, Socket Option- Reuse Address
    //&opt and opt = 1 means we are turning this setting on

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port); //Host to network short: converts 16 bit number stored in the computer's native format to the universal format
    serverAddr.sin_addr.s_addr = INADDR_ANY;


    //bind assigns a specific permanent id to the socket
    //a servver needs a permanent unchanging address so that client computers know where to send their data
    //it takes in the integer id of the socket ie server_socket
    //the struct that contains the exact IP address and port we want
    //size of that struct in bytes
    if(bind(server_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0){
        std::cerr << "Error binding server socket\n";
        return;
    }


    if(listen(server_socket, 10) < 0){
        std::cerr<< "Error listening on server socket\n";
    }

    std::cout<<"Redis sever listening on Port " << port << "\n";

    //accepting input from the client sockets
    std::vector<std::thread> threads;
    RedisCommandHandler cmdHandler;

    while(running){
        int client_socket = accept(server_socket, nullptr, nullptr);
        if(client_socket < 0){
            if(running) std::cerr<< "Error accepting client communication\n";
            break;
        }

        threads.emplace_back([client_socket, &cmdHandler](){
            char buffer[1024];
            while(true){
                memset(buffer, 0, sizeof(buffer));

                int bytes = recv(client_socket, buffer, sizeof(buffer)-1, 0);
                if(bytes <= 0) break;;

                std::string request(buffer, bytes);
                std::string response = cmdHandler.processCommand(request);

                send(client_socket, response.c_str(), response.size(), 0);
            }
            close(client_socket);
        });
    }

    for(auto& t: threads){
        if(t.joinable()) t.join();
    }

    //shutdown
    //before shutdown persiste the database
    if(RedisDatabase::getInstance().dump("dump.my_rdb")){
        std::cout << "Database dumper to dump.my_rdb \n";
    }
    else{
        std::cerr<< "Error dumping database \n";
    }
}