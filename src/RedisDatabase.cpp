#include "../include/RedisDatabase.h"

#include <fstream>
//used to read and write into files
#include <sstream>

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}

//common operations
bool RedisDatabase::flushAll(){
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store.clear();
    list_store.clear();
    hash_store.clear();
    return true;
}

//key value operations
void RedisDatabase::set(const std::string& key, const std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    kv_store[key] = value;
}

bool RedisDatabase::get(const std::string& key, std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    auto it = kv_store.find(key);
    if(it != kv_store.end()){
        value = it->second;
        return true;
    }
    return false;
}

std::vector<std::string> RedisDatabase::keys(){
    std::lock_guard<std::mutex> lock(db_mutex);
    std::vector<std::string> result;
    for(const auto& pair: kv_store){
        result.push_back(pair.first);
    }
    for(const auto& pair: list_store){
        result.push_back(pair.first);
    }
    for(const auto& pair: hash_store){
        result.push_back(pair.first);
    }

    return result;
}

std::string RedisDatabase::type(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    if(kv_store.find(key) != kv_store.end())
        return "string";
    if(list_store.find(key) != list_store.end())
        return "list";
    if(hash_store.find(key) != hash_store.end())
        return "hash";
    return "none";
}

bool RedisDatabase::del(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erase = false;
    erase |= (kv_store.erase(key) > 0);
    erase |= (list_store.erase(key) > 0);
    erase |= (hash_store.erase(key) > 0);

    return erase;
}

bool RedisDatabase::expire(const std::string& key, const std::string& seconds){
    std::lock_guard<std::mutex> lock(db_mutex);

    bool exists = (kv_store.find(key) != kv_store.end()) ||
                (list_store.find(key) != list_store.end()) ||
                (hash_store.find(key) != hash_store.end());

    if(!exists) return false;

    int seconds_int = std::stoi(seconds);
    expiry_map[key] = std::chrono::steady_clock::now() + std::chrono::seconds(seconds_int);
    return true;
}

bool RedisDatabase::rename(const std::string& oldKey, const std::string& newKey){
    std::lock_guard<std::mutex> lock(db_mutex);
    bool found = false;

    auto itKv = kv_store.find(oldKey);
    if(itKv != kv_store.end()){
        kv_store[newKey] = itKv -> second;
        kv_store.erase(itKv);
        found = true;
    }

    auto itlist = list_store.find(oldKey);
    if(itlist != list_store.end()){
        list_store[newKey] = itlist -> second;
        list_store.erase(itlist);
        found = true;
    }

    auto ithash = hash_store.find(oldKey);
    if(ithash != hash_store.end()){
        hash_store[newKey] = ithash -> second;
        hash_store.erase(ithash);
        found = true;
    }

    return found;
}


//PERSISTANCE
/*
Memory -> File - dump()
File -> Memory - load()

K = Key value
L = List
H = Hash
*/
//we will use mutex for thread safety
/*
working
*/

bool RedisDatabase::dump(const std::string& filename){

    std::lock_guard<std::mutex> lock(db_mutex);
    //stores the data in raw unformmated bytes exactly as its stored in computer memory
    //writes to the file if it is open
    std::ofstream ofs(filename, std::ios::binary);
    if(!ofs) return false;

    //we use const here since we dont want to edit the data inside this for loop
    for(const auto& kv: kv_store){
        ofs << "K " << kv.first << " " << kv.second << "\n";
    }
    for(const auto& kv: list_store){
        ofs << "L " << kv.first;
        for(const auto& item: kv.second){
            ofs << " " << item;
        }
        ofs << "\n";
    }
    for(const auto& kv: hash_store){
        ofs << "H " << kv.first;
        for(const auto& field_val: kv.second){
            ofs << " " << field_val.first << ":" << field_val.second;
        }
        ofs << "\n";
    }

    return true;
}

bool RedisDatabase::load(const std::string& filename){

    std::lock_guard<std::mutex> lock(db_mutex);
    //input stream instead of output stream
    std::ifstream ifs(filename, std::ios::binary);
    if(!ifs) return false;

    kv_store.clear();
    list_store.clear();
    hash_store.clear();

    std::string line;
    while(std::getline(ifs, line)){
        std::istringstream iss(line);
        char type;
        iss >> type;

        if(type == 'K'){
            std::string key, value;
            iss >> key >> value;
            kv_store[key] = value;
        }
        else if(type == 'L'){
            std::string key;
            iss >> key;
            std::string item;
            std::vector<std::string> list;

            while(iss >> item){
                list.push_back(item);
            }
            list_store[key] = list;
        }
        else if(type == 'H'){
            std::string key;
            iss >> key;

            std::unordered_map<std::string, std::string> hash;
            std::string pair;

            while(iss >> pair){
                auto pos = pair.find(':');
                if(pos != std::string::npos){
                    std::string field = pair.substr(0, pos);
                    std::string value = pair.substr(pos+1);
                    hash[field] = value;
                }
            }
            hash_store[key] = hash;
        }
    }

    return true;
}