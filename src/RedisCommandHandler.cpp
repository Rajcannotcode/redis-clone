#include <../include/RedisCommandHandler.h>
#include <../include/RedisDatabase.h>

#include <vector>
#include <sstream>
//header which allows us to read and write data directly to a string hidden in computer memory
#include <algorithm>
//for the transform function

// RESP parser:
//the format of our input
// *2\r\n$4\r\n\PING\r\n$4\r\n\TEST\r\n
// *2 -> array has two elements
// $4 -> next string has 4 characters
// after this we will have PING and TEST


std::vector<std::string> parseRespCommands(const std::string &input){
    std::vector<std::string> tokens;
    
    if(input.empty()) return tokens;

    //if it doesnt start with '*', fallback to splitting by whitespace
    if(input[0] != '*'){
        std::istringstream iss(input); //used only for reading data out of a string
        //this splits the input via whitespace
        std::string token;

        while(iss >> token) tokens.push_back(token);
        //while we are able to input tokens, we will add them to our vector
        return tokens;
    }

    size_t pos = 0;
    //Expect * followed by numberr of elements
    if(input[pos] != '*') return tokens;
    pos++; //skip *

    //crlf = Carriage return '\r', line feed '\n'
    size_t crlf = input.find("\r\n", pos);
    if(crlf == std::string::npos) return tokens;

    int numElements = std::stoi(input.substr(pos, crlf-pos));
    pos = crlf + 2;

    for(int i  = 0; i<numElements; i++){
        if(pos >= input.size() || input[pos] != '$') break; //format error
        pos++; //skip $

        crlf = input.find("\r\n", pos);
        if(crlf == std::string::npos) break;

        int len = std::stoi(input.substr(pos, crlf-pos));
        pos = crlf + 2;

        if(pos + len > input.size()) break;
        std::string token = input.substr(pos, len);
        tokens.push_back(token);
        pos += len + 2; //skip token and crlf
    }

    return tokens;
}

RedisCommandHandler::RedisCommandHandler(){}

std::string RedisCommandHandler::processCommand(const std::string& commandLine){
    //use RESP parser
    auto tokens = parseRespCommands(commandLine);
    if(tokens.empty()) return "-Error: Empty command\r\n";

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;
    //used to output stuff on the terminal

    //Connect to database
    RedisDatabase& db = RedisDatabase::getInstance();


    //Check commands
    //common commands
    //used to check connection
    if(cmd == "PING"){
        response << "+PONG\r\n";
    }
    //used to respond with the second string inputted
    else if(cmd == "ECHO"){
        if(tokens.size() < 2) response << "-Error: ECHO requires a message\r\n";
        else{
            response << "+" << tokens[1] << "\r\n";
        }
    }
    //clears all stored keys
    else if(cmd == "FLUSHALL"){
        db.flushAll();
        response << "+OK\r\n";
    }

    //key value operations
    //caching user session , information or configuration setting
    else if(cmd == "SET"){
        if(tokens.size() < 3) response << "-Error: SET requires key and value\r\n";
        else{
            db.set(tokens[1], tokens[2]);
            response << "+OK\r\n";
        }
    }
    //retrieving stored data
    else if(cmd == "GET"){
        if(tokens.size() < 2) response << "-Error: GET requires key\r\n";
        else{
            std::string value;
            if(db.get(tokens[1], value)){
                response << "$" << value.size() << "\r\n" << value << "\r\n";
            }
            else response << "$-1\r\n";
        }
    }
    //lists out all the keys
    else if(cmd == "KEYS"){
        std::vector<std::string> allKeys = db.keys();
        response << "*" << allKeys.size() <<  "\r\n";
        for(const auto& key: allKeys){
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
    }
    //check if the type is string, list or hash
    else if(cmd == "TYPE"){
        if(tokens.size() < 2) response << "-Error: TYPE requires ket\r\n";
        else{
            response << "+" << db.type(tokens[1]) << "\r\n";
        }
    }
    //remove keys which are no longer valid
    else if(cmd == "DEL" || cmd == "UNLINK"){
        if(tokens.size() < 2) response << "-Error: " << cmd << " requires key\r\n";
        else{
            bool res = db.del(tokens[1]);
            response << ":" << (res ? 1 : 0) << "\r\n";
        }
    }
    //set a timeout on the keys for caching
    //cache automatically evicts old data
    else if(cmd == "EXPIRE"){
        if(tokens.size() < 3) response << "-Error: EXPIRE requires key and time in seconds\r\n";
        else{
            if(db.expire(tokens[1], tokens[2]))
                response << "+OK\r\n";
        }
    }
    //change  the key for a given key-value pair
    else if(cmd == "RENAME"){
        if(tokens.size() < 3) response << "-Error: RENAME requires old and new key name\r\n";
        else{
            if(db.rename(tokens[1], tokens[2]))
                response << "+OK\r\n";
        }
    }

    //list operations

    //hash oprrations
    else{
        response << "0-Error:Unknown Command\r\n";
    }

    return response.str();
}