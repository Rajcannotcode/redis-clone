#include <../include/CLI.h>

#include <iostream>
#include <string>

int main(int argc, char* argv[]){
    
    std::string host = "127.0.0.1";
    int port = 6379;
    int i = 1;

    std::vector<std::string> commandArgs;

    //Parse command line args for -h and -p
    while(i < argc){
        std::string arg = argv[i];
        if(arg == "-h" && i+1 < argc){
            host = argv[++i];
        }
        else if(arg == "-p" && i+1 < argc){
            port = std::stoi(argv[++i]);
        }else{
            while(i < argc){
                commandArgs.push_back(argv[i]);
                i++;
            }
        }
        ++i;
    }

    //Handle REPL and one-shot command nodes
    CLI cli(host, port);
    cli.run(commandArgs);

    return 0;
}


/*
Command-Lne argument parting
    -h <host> default: 127:0:0:1
    -p <port> default: 6379

If no args;
 launch interactive REPL mode
 
OOP
    RedisClient, CommandHandler, ResponseParser, CLI

Establish TCP connection to Redis (RedisClient)
    Berkely socket to open TCP connection
    IPv4 and IPv6 'getaddrinfo'

Parsing and command formatting (CommandHandler)
    Split user input
    Convert commadns into RESP format:

    '''
    *3\r\n
    $3\r\n\SET\r\n
    $8\\r\nFLUSHALL\r\n
    '''

Handle Redis Response (Response Parser)
    Read server responses and parses RESP types
    +OK, -Error, :100, $5\R\Nhello-> Bulk string
    *2\r\n\$3\r\nfoo\r\n$\r\nbar ->Array response
    
Implement interactive REPL (CLI)
    Run loop: user-input, valid redis commands, send commands to server, display parsed response
    support: help, quit

main.cpp: parse command line args, instantiate CLI and launch in REPL mode

Socket Programming:
Protocol Handling
OOP principles
CLI development

*/