#ifndef RESPONSE_PARSER_H
#define RESPONSE_PARSER_H


#include <string>


class ResponseParser{
public:
    //read from the given circuit and return parsed response a string, retrun "" in failure
    static std::string parseResponse(int sockfd);

private:
//Redis Serialization protocol 2
    static std::string parseSimpleString(int sockfd);
    static std::string parseSimpleErrors(int sockfd);
    static std::string parseInteger(int sockfd);
    static std::string parseBulkString(int sockfd);
    static std::string parseArray(int sockfd);
};

#endif