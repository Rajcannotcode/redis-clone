#ifndef REDIS_SERVER_H //if not defined yet
#define REDIS_SERVER_H
//ifndef is called an include guard
//it prevents a single header file from being included in the same code file]
//prevents multiple definition error
//can also use #pragma once

#include <string>
#include <atomic>
//atomic provides thread safe lock free programming
//primary job is to ensure that when multiple threads are reading and modifying the variable
//the data doesnt get corrupted

class RedisServer{
public:
    RedisServer(int port);

    void run();

    void shutdown();

private:
    int port;
    int server_socket;
    std::atomic<bool> running;


    //setup signal handling for graceful shutdown (crtl + c)
    void setupSignalHandler();
};


#endif //ends the if condition


/*
Why split the class into .h and .cpp
-.h acts as the interface, shows what the class does, what paarameters it takes, what it returns
-.cpp is the implementation
-faster compilation time as when .cpp is changed it only recompiles that cpp file, while .h is used to copy the entire content into that file
*/