#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <vector>
#include <string>

class CommandHandler{

public:
    //split command into tokens
    static std::vector<std::string> splitArgs(const std::string& input);

    //build a RESP command from the vector arguments
    static std::string buildRESPcommand(const std::vector<std::string> &args);

private:
};

#endif