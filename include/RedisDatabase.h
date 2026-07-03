#ifndef REDIS_DATABASE_H
#define REDIS_DATABASE_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <chrono>

class RedisDatabase{

public:
    //Get singleton instance
    static RedisDatabase& getInstance();

    //common commands
    bool flushAll();

    //key value operations
    void set(const std::string& key, const std::string& value);
    bool get(const std::string& key, std::string& value);
    std::vector<std::string> keys();
    std::string type(const std::string& key);
    bool del(const std::string& key);
    bool expire(const std::string& key, const std::string& seconds);
    bool rename(const std::string& oldKey, const std::string& newKey);


    //persistence: dump/load databse from file
    bool dump(const std::string& filename);
    bool load(const std::string& filename);

private:

    //force the compiler to use the default consturctor and destructor
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    //creates brand new database by copying existing one
    RedisDatabase(const RedisDatabase&) = default;

    //forbids overwriting one database by using another
    RedisDatabase& operator = (const RedisDatabase&) = delete;

    //mutex acts like a lock on a shared resource, ensuring only one resource can access it at a time
    std::mutex db_mutex;

    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;

    std::unordered_map<std::string, std::chrono::steady_clock::time_point> expiry_map;
};


#endif