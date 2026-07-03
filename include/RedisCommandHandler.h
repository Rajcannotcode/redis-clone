#ifndef REDIS_COMMAND_HANDLER_H
#define REDIS_COMMAND_HANDLER_H

#include <string>

class RedisCommandHandler {

public:
    RedisCommandHandler();
    //process a command from a client and return RESP-formatter response

    std::string processCommand(const std::string& commandline);
};

#endif
