#include <httplib.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>

class LANSyncServer {
private:
    httplib::Server server;
    std::string storage_path;
    size_t max_file_size;
    std::unordered_map<std::string, std::string> active_sessions; 
    
public:
    LANSyncServer(const std::string& path, size_t max_size = 100 * 1024 * 1024) 
        : storage_path(path), max_file_size(max_size) {
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
    
    bool write_file_safely(const std::string& path, const std::string& content);
    
    void ensure_storage_directory();
};