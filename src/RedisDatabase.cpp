#include "../include/RedisDatabase.h"

#include <fstream>
//used to read and write into files
#include <sstream>
#include <thread>
#include <unistd.h>

RedisDatabase& RedisDatabase::getInstance(){
    static RedisDatabase instance;
    return instance;
}

RedisDatabase::RedisDatabase(){
    //start the background active eviction thread
    std::thread(&RedisDatabase::activeEvictionLoop, this).detach();
}

//sweeps the database every second for expired keys
void RedisDatabase::activeEvictionLoop(){
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::lock_guard<std::mutex> lock(db_mutex);
        auto now = std::chrono::steady_clock::now();
        
        for(auto it = expiry_map.begin(); it != expiry_map.end(); ){
            if(now > it->second){
                //key is expired, erase it from all stores
                kv_store.erase(it->first);
                list_store.erase(it->first);
                hash_store.erase(it->first);
                it = expiry_map.erase(it);
            } else {
                ++it;
            }
        }
    }
}

//checks a single key for passive eviction. must be called while db_mutex is locked
bool RedisDatabase::isExpired(const std::string& key){
    auto it = expiry_map.find(key);
    if(it == expiry_map.end()) return false;

    if(std::chrono::steady_clock::now() > it->second){
        kv_store.erase(key);
        list_store.erase(key);
        hash_store.erase(key);
        expiry_map.erase(it);
        return true;
    }
    return false;
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
    if(isExpired(key)) return false; //passive eviction

    auto it = kv_store.find(key);
    if(it != kv_store.end()){
        value = it->second;
        return true;
    }
    return false;
}

std::string RedisDatabase::type(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    if(isExpired(key)) return "none"; //passive eviction

    if(kv_store.find(key) != kv_store.end()) return "string";
    if(list_store.find(key) != list_store.end()) return "list";
    if(hash_store.find(key) != hash_store.end()) return "hash";
    return "none";
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


bool RedisDatabase::del(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    bool erase = false;
    erase |= (kv_store.erase(key) > 0);
    erase |= (list_store.erase(key) > 0);
    erase |= (hash_store.erase(key) > 0);
    
    //clean up the expiry map too
    expiry_map.erase(key);

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

//list operations
int RedisDatabase::lpush(const std::string& key, const std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    isExpired(key); //passive eviction

    //insert at the front of the vector
    list_store[key].insert(list_store[key].begin(), value);
    return list_store[key].size();
}

int RedisDatabase::rpush(const std::string& key, const std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    isExpired(key);

    //insert at the back of the vector
    list_store[key].push_back(value);
    return list_store[key].size();
}

bool RedisDatabase::lpop(const std::string& key, std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    isExpired(key);

    auto it = list_store.find(key);
    if(it == list_store.end() || it->second.empty()) return false;

    value = it->second.front();
    it->second.erase(it->second.begin());

    //clean up empty lists to save memory
    if(it->second.empty()) list_store.erase(it);
    return true;
}

bool RedisDatabase::rpop(const std::string& key, std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    isExpired(key);

    auto it = list_store.find(key);
    if(it == list_store.end() || it->second.empty()) return false;

    value = it->second.back();
    it->second.pop_back();

    if(it->second.empty()) list_store.erase(it);
    return true;
}

std::vector<std::string> RedisDatabase::lrange(const std::string& key, int start, int stop){
    std::lock_guard<std::mutex> lock(db_mutex);
    isExpired(key);

    std::vector<std::string> res;
    
    auto it = list_store.find(key);
    if(it == list_store.end()) return res;

    int len = it->second.size();
    
    //convert negative indices (e.g. -1 is the last element)
    if(start < 0) start = len + start;
    if(stop < 0) stop = len + stop;

    //clamp to boundaries
    if(start < 0) start = 0;
    if(stop >= len) stop = len - 1;
    if(start > stop || start >= len) return res;

    for(int i = start; i<=stop; i++){
        res.push_back(it->second[i]);
    }
    
    return res;
}

//hash operations
bool RedisDatabase::hset(const std::string& key, const std::string& field, const std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    isExpired(key);

    //check if field exists before overwriting to return correct integer flag
    bool isNew = (hash_store[key].find(field) == hash_store[key].end());
    hash_store[key][field] = value;
    return isNew;
}

bool RedisDatabase::hget(const std::string& key, const std::string& field, std::string& value){
    std::lock_guard<std::mutex> lock(db_mutex);
    isExpired(key);

    auto it = hash_store.find(key);
    if(it == hash_store.end()) return false;

    auto fieldIt = it->second.find(field);
    if(fieldIt == it->second.end()) return false;

    value = fieldIt->second;
    return true;
}

std::vector<std::string> RedisDatabase::hgetAll(const std::string& key){
    std::lock_guard<std::mutex> lock(db_mutex);
    isExpired(key);

    std::vector<std::string> res;
    
    auto it = hash_store.find(key);
    if(it == hash_store.end()) return res;

    //returns flattened list of field, value, field, value
    for(const auto& pair: it->second){
        res.push_back(pair.first);
        res.push_back(pair.second);
    }
    
    return res;
}


//PERSISTANCE
/*
Memory -> File - dump()
File -> Memory - load()

K = Key value
L = List
H = Hash
E = expiration
*/
//we will use mutex for thread safety
/*
working
*/

bool RedisDatabase::dump(const std::string& filename){
    std::lock_guard<std::mutex> lock(db_mutex);
    //stores the data in raw unformmated bytes exactly as its stored in computer memory
    std::ofstream ofs(filename, std::ios::binary);
    if(!ofs) return false;

    auto now = std::chrono::steady_clock::now();

    //dump expirations first
    for(const auto& exp: expiry_map){
        if(exp.second > now){
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(exp.second - now).count();
            ofs << "E " << exp.first << " " << ms << "\n";
        }
    }

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
    expiry_map.clear(); //clear old expirations on fresh load

    std::string line;
    auto now = std::chrono::steady_clock::now();

    while(std::getline(ifs, line)){
        std::istringstream iss(line);
        char type;
        iss >> type;

        if(type == 'E'){
            std::string key;
            long long ms;
            iss >> key >> ms;
            expiry_map[key] = now + std::chrono::milliseconds(ms);
        }
        else if(type == 'K'){
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

bool RedisDatabase::bgdump(const std::string& filename){
    // lock to ensure we don't fork while memory is being actively mutated
    std::lock_guard<std::mutex> lock(db_mutex);

    pid_t pid = fork();
    if(pid < 0){
        return false; // fork failed
    }
    
    if(pid == 0){
        // child process: we have a perfect point-in-time snapshot of memory
        std::ofstream ofs(filename, std::ios::binary);
        if(ofs){
            auto now = std::chrono::steady_clock::now();

            for(const auto& exp: expiry_map){
                if(exp.second > now){
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(exp.second - now).count();
                    ofs << "E " << exp.first << " " << ms << "\n";
                }
            }
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
        }
        // exit immediately using _exit to prevent running standard C++ destructors 
        // which could mess up the parent's file descriptors or memory states
        _exit(0); 
    }

    // parent process: return immediately, lock_guard goes out of scope and unlocks
    return true;
}