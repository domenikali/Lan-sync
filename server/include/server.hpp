#include <httplib.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include "storage_manager.hpp"
#include "db_manager.hpp"
#include "hash_utils.hpp"

class LANSyncServer {
private:
    httplib::Server server;
    std::string ssd_cache_path;
    std::string hdd_storage_path;
    size_t max_file_size;
    std::unordered_map<std::string, std::string> active_sessions; 
    StorageManager * storage_manager;
    DBManager* db_manager;
    
public:
    LANSyncServer(const std::string& ssd_path,const std::string& hdd_path, size_t max_size = 100 * 1024 * 1024) 
        : ssd_cache_path(ssd_path),hdd_storage_path(hdd_path), max_file_size(max_size) {

        
        db_manager = new DBManager(hdd_storage_path + "/lansync.db"); 
        storage_manager = new StorageManager(hdd_storage_path, ssd_cache_path, db_manager);
        setup_routes();
        
        ensure_storage_directory();
    }
    
    void setup_routes();
    
    void handle_file_upload(const httplib::Request& req, httplib::Response& res);
    
    void handle_file_download(const std::string& filename, const httplib::Request& req, httplib::Response& res);    
    
    void handle_list_files(const httplib::Request& req, httplib::Response& res);    
   
    void handle_file_delete(const std::string& filename, const httplib::Request& req, httplib::Response& res);
    
    void handle_file_info(const std::string& filename, const httplib::Request& req, httplib::Response& res);

    void start_server(const std::string& host, int port );
    
private:
    bool authenticate_request(const httplib::Request& req);
    
    std::string sanitize_filename(const std::string& filename);
    
    bool write_file_safely(const std::string& filename,const std::string& path, const std::string& content);
    
    void ensure_storage_directory();
};