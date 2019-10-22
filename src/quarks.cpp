#include <quarks.hpp>
#include <v8engine.hpp>

#include "rocksdb/db.h"

using namespace Quarks;

Core Core::_Instance;
Matrix Matrix::_Instance;

rocksdb::DB* db = nullptr;
rocksdb::Status dbStatus;

void initDB(){
    rocksdb::Options options;
    options.create_if_missing = true;
    dbStatus = rocksdb::DB::Open(options, "quarks_db", &db);
        
}

void closeDB(){
    // close the database
    delete db;
}

int wildcmp(const char *wild, const char *string) {
    // Written by Jack Handy - <A href="mailto:jakkhandy@hotmail.com">jakkhandy@hotmail.com</A>
    const char *cp = NULL, *mp = NULL;
    
    while ((*string) && (*wild != '*')) {
        if ((*wild != *string) && (*wild != '?')) {
            return 0;
        }
        wild++;
        string++;
    }
    
    while (*string) {
        if (*wild == '*') {
            if (!*++wild) {
                return 1;
            }
            mp = wild;
            cp = string+1;
        } else if ((*wild == *string) || (*wild == '?')) {
            wild++;
            string++;
        } else {
            wild = mp;
            string = cp++;
        }
    }
    
    while (*wild == '*') {
        wild++;
    }
    return !*wild;
}

Core::Core(){
    initDB();
}

Core::~Core(){
    closeDB();
}

void Core::setEnvironment(int argc, std::string argv){
    _argv = argv;
    _argc = argc;
}

std::string Core::putJson(std::string key, crow::json::rvalue& x) {
    
    //_Core[key] = std::move(x);
    crow::json::wvalue w = std::move(x);
    std::string value = crow::json::dump(w);
    
    rocksdb::Slice keySlice = key;
        
    // modify the database
    if (dbStatus.ok()){
        rocksdb::Status status = db->Put(rocksdb::WriteOptions(), keySlice, value);
        if(!status.ok()){
            key = "";
        }
    
    }else{
        key = "";
    }
    
    
    return  key;
    
}

bool Core::getJson(std::string key, crow::json::wvalue& out){
    
    bool ret = false;
    
    /*std::map<std::string, crow::json::rvalue>::iterator it = _Core.find(key);
    if (it != _Core.end()){
        out = it->second;
        ret = true;
    }*/
    if (dbStatus.ok()){
        std::string value;
        rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
    
        if(status.ok()){
            out =  crow::json::load(value);
            ret = true;
        }
    }
    
    return ret;
    
}

bool Core::getValue(std::string key, std::string& value){
    
    bool ret = false;
    
    if (dbStatus.ok()){
        
        rocksdb::Status status = db->Get(rocksdb::ReadOptions(), key, &value);
        
        if(status.ok()){
            ret = true;
        }
    }
    
    return ret;
    
}

bool Core::findJson(std::string wild, std::vector<crow::json::wvalue>& matchedResults) {

    /*for (std::map<std::string, crow::json::rvalue>::iterator it = _Core.begin();
         it != _Core.end(); ++it){
            if(wildcmp(wild.c_str(), it->first.c_str()) ){
                crow::json::wvalue w;
                w = it->second;
                CROW_LOG_INFO << "w : " << crow::json::dump(w);
                
                matchedResults.push_back(std::move(w));
            }
    }*/
    
    if (dbStatus.ok()){
    // create new iterator
        rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
        
        // iterate all entries
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            CROW_LOG_INFO << "iterate : " << it->key().ToString()
            << ": " << it->value().ToString() ; //<< endl;
            
            if(wildcmp(wild.c_str(), it->key().ToString().c_str())){
                crow::json::wvalue w;
                w = crow::json::load(it->value().ToString());
                CROW_LOG_INFO << "w : " << crow::json::dump(w);
                
                matchedResults.push_back(std::move(w));
                
            }
        }
        //assert(it->status().ok());  // check for any errors found during the scan
        
        return it->status().ok();
    }
    
    return false;
    
}

bool Core::filterJson(crow::json::rvalue& filter,
                       std::vector<crow::json::wvalue>& matchedResults) {
    
    
    if (dbStatus.ok()){
        
        Core& core = *this;
        
        v8Engine ve([](std::string log){
            CROW_LOG_INFO << log.c_str();
        });
        
        auto funcOnV8EngineReady = [&core, &ve, &filter, &matchedResults](v8Engine::v8Context& v8Ctx){
            
            std::string sfilter =  crow::json::dump(filter);
            
            CROW_LOG_INFO << "filter : " << sfilter;
            
            std::string wild = filter["keys"].s();
            CROW_LOG_INFO << "keys : " << wild.c_str();
            
            std::string filterParams = filter["filterparams"].s();
            
            crow::json::wvalue subFilter;
            subFilter["subfilter"] = filter["filter"];
            //crow::json::rvalue subFilter = filter["filter"];
            CROW_LOG_INFO << "subfilter : " << crow::json::dump(subFilter);
            
            crow::json::wvalue wMap;
            wMap["map"]= std::move(subFilter["subfilter"]);
            CROW_LOG_INFO << "map : " << crow::json::dump(wMap["map"]);
            
            std::string wx = crow::json::dump(wMap["map"]);
            crow::json::rvalue map = crow::json::load(wx);
            std::string mapField = map["map"]["field"].s();
            std::string mapAs = map["map"]["as"].s();
            // create new iterator
            rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
            
            // iterate all entries
            for (it->SeekToFirst(); it->Valid(); it->Next()) {
                
                std::string val = it->value().ToString();
                
                CROW_LOG_INFO << "iterate : " << it->key().ToString()
                << ": " <<  val; //<< endl;
                
                if(wildcmp(wild.c_str(), it->key().ToString().c_str())){
                    crow::json::rvalue r;
                    r = crow::json::load(it->value().ToString());
                    
                    CROW_LOG_INFO << "r : " << val;
                    
                    std::string mapKey = r[mapField.c_str()].s();
                    crow::json::wvalue mapJson;
                    
                    crow::json::wvalue w = r;
                    if(core.getJson(mapKey, mapJson)){
                        w[mapAs] = std::move(mapJson);
                    }
                    
                    //std::string allow = ve.invoke("matcher", sfilter.c_str(), //filterParams.c_str());
                    
                    std::string allow = ve.invoke(v8Ctx, "matcher", val, filterParams.c_str());
                    if(atoi(allow.c_str()) == 1){
                        matchedResults.push_back(std::move(w));
                    }
                    
                }
            }
            
            ve.setResult(it->status().ok());
        }; // funcOnReady
        
        ve.run(funcOnV8EngineReady);
        
        //assert(it->status().ok());  // check for any errors found during the scan
        
        return ve.getResult();
        
    }
    
    return false;
    
}
