#include "../include/CLI.h"
#include "../include/ResponseParser.h"


static std::string trim(const std::string& s){

    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    if(start == std::string::npos) return "";

    size_t end = s.find_last_not_of(" \t\n\r\f\v");
    return s.substr(start, end - start + 1);

}

CLI::CLI(const std::string & host, int port) 
    :redisClient(host, port), host(host), port(port){}

void CLI::run(const std::vector<std::string>& args){
    if(!redisClient.connectToServer()){
        return;
    }

    executeCommand(args);

    std::cout << "Connected to Redis at " << redisClient.getSocketFD() << "\n";

    while(true){
        std::cout << host << ":" << port << ">";
        std::cout.flush();
        std::string line;

        if(!std::getline(std::cin, line)) break;

        line = trim(line);
        if(line.empty()) continue;
        if(line == "quit" || line == "exit"){
            std::cout << "Goodbye. \n";
            break;
        } 
        if(line == "help"){
            std::cout << "Of course, help yourself\n";
        }

        //split commands into tokens
        std::vector<std::string> args = CommandHandler::splitArgs(line);

        if(args.empty()) continue;

        std::string command = CommandHandler::buildRESPcommand(args);

        if(!redisClient.sendCommand(command)){
            std::cerr<< "(Error) Failed to send command \n";
            break;
        }

        //Parse and print response
        std::string response = ResponseParser::parseResponse(redisClient.getSocketFD());

        std::cout << response << "\n";
    }
}

void CLI::executeCommand(const std::vector<std::string>& args){
    if(args.empty()) return;

    std::string command = CommandHandler::buildRESPcommand(args);

    if(!redisClient.sendCommand(command)){
        std:: cerr << "(Error) Failed to send command \n";
        return;
    }

    //parse and print response
    std::string response = ResponseParser::parseResponse(redisClient.getSocketFD());
    std::cout << response << "\n";
}