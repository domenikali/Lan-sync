#include "server.hpp"

void LANSyncServer::setup_routes() {
    server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        return httplib::Server::HandlerResponse::Unhandled;
    });
    
    server.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        return; 
    });
    
    server.Post("/api/upload", [this](const httplib::Request& req, httplib::Response& res) {
        handle_file_upload(req, res);
    });
    
    server.Get("/api/download/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string filename = req.matches[1];
        handle_file_download(filename, req, res);
    });
    
    server.Get("/api/files", [this](const httplib::Request& req, httplib::Response& res) {
        handle_list_files(req, res);
    });
    
    server.Delete("/api/files/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string filename = req.matches[1];
        handle_file_delete(filename, req, res);
    });
    
    server.Get("/api/info/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string filename = req.matches[1];
        handle_file_info(filename, req, res);
    });
}

void LANSyncServer::handle_file_upload(const httplib::Request& req, httplib::Response& res) {
   
    
    std::string filename = "upload_" + std::to_string(time(nullptr));
    auto it = req.headers.find("X-Filename");
    if (it != req.headers.end()) {
        filename = it->second;
    }
    
    if (req.body.size() > max_file_size) {
        res.status = 413;
        res.set_content("{\"error\": \"File too large\"}", "application/json");
        return;
    }
    
    filename = sanitize_filename(filename);
    if (filename.empty()) {
        res.status = 400;
        res.set_content("{\"error\": \"Invalid filename\"}", "application/json");
        return;
    }
    
    std::string full_path = storage_path + "/" + filename;
    if (write_file_safely(full_path, req.body)) {
        res.status = 200;
        res.set_content("{\"message\": \"Upload successful\", \"filename\": \"" + filename + "\"}", 
                      "application/json");
    } else {
        res.status = 500;
        res.set_content("{\"error\": \"Failed to save file\"}", "application/json");
    }
}

void LANSyncServer::handle_file_download(const std::string& filename, const httplib::Request& req, httplib::Response& res) {
    
    std::string safe_filename = sanitize_filename(filename);
    std::string full_path = storage_path + "/" + safe_filename;
    
    if (!std::filesystem::exists(full_path)) {
        res.status = 404;
        res.set_content("{\"error\": \"File not found\"}", "application/json");
        return;
    }
    
    std::ifstream file(full_path, std::ios::binary);
    if (!file) {
        res.status = 500;
        res.set_content("{\"error\": \"Cannot read file\"}", "application/json");
        return;
    }

    auto file_size = std::filesystem::file_size(full_path);

    auto file_stream = std::make_shared<std::ifstream>(std::move(file));
    
    res.set_content_provider(
        file_size,
        "application/octet-stream",
        [file_stream](size_t offset, size_t length, httplib::DataSink& sink) {
            std::vector<char> buffer(std::min(length, size_t(8192)));
            
            file_stream->seekg(offset);
            file_stream->read(buffer.data(), buffer.size());
            size_t bytes_read = file_stream->gcount();
            
            if (bytes_read > 0) {
                sink.write(buffer.data(), bytes_read);
            }
            
            return bytes_read > 0;
        }
    );
    
    res.set_header("Content-Disposition", "attachment; filename=\"" + safe_filename + "\"");
}

void LANSyncServer::handle_list_files(const httplib::Request& req, httplib::Response& res) {
    
    std::string json_response = "{\"files\": [";
    bool first = true;
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(storage_path)) {
            if (entry.is_regular_file()) {
                if (!first) json_response += ",";
                json_response += "{";
                json_response += "\"name\": \"" + entry.path().filename().string() + "\",";
                json_response += "\"size\": " + std::to_string(entry.file_size()) + ",";
                json_response += "\"modified\": " + std::to_string(
                    std::chrono::duration_cast<std::chrono::seconds>(
                        entry.last_write_time().time_since_epoch()).count());
                json_response += "}";
                first = false;
            }
        }
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content("{\"error\": \"Cannot list files\"}", "application/json");
        return;
    }
    
    json_response += "]}";
    res.set_content(json_response, "application/json");
}

void LANSyncServer::handle_file_delete(const std::string& filename, const httplib::Request& req, httplib::Response& res) {

    std::string safe_filename = sanitize_filename(filename);
    std::string full_path = storage_path + "/" + safe_filename;
    
    if (std::filesystem::remove(full_path)) {
        res.status = 200;
        res.set_content("{\"message\": \"File deleted\"}", "application/json");
    } else {
        res.status = 404;
        res.set_content("{\"error\": \"File not found\"}", "application/json");
    }
}

void LANSyncServer::handle_file_info(const std::string& filename, const httplib::Request& req, httplib::Response& res) {
    
    std::string safe_filename = sanitize_filename(filename);
    std::string full_path = storage_path + "/" + safe_filename;
    
    if (!std::filesystem::exists(full_path)) {
        res.status = 404;
        res.set_content("{\"error\": \"File not found\"}", "application/json");
        return;
    }
    
    try {
        auto file_size = std::filesystem::file_size(full_path);
        auto last_write = std::filesystem::last_write_time(full_path);
        
        std::string json_response = "{";
        json_response += "\"name\": \"" + safe_filename + "\",";
        json_response += "\"size\": " + std::to_string(file_size) + ",";
        json_response += "\"modified\": " + std::to_string(
            std::chrono::duration_cast<std::chrono::seconds>(
                last_write.time_since_epoch()).count());
        json_response += "}";
        
        res.set_content(json_response, "application/json");
    } catch (const std::exception& e) {
        res.status = 500;
        res.set_content("{\"error\": \"Cannot get file info\"}", "application/json");
    }
}

void LANSyncServer::start_server(const std::string& host = "0.0.0.0", int port = 8080) {
    std::cout << "Starting LAN Drive server on " << host << ":" << port << std::endl;
    std::cout << "Storage path: " << storage_path << std::endl;
    server.listen(host, port);
}

bool LANSyncServer::authenticate_request(const httplib::Request& req) {
    
    //TODO: Implement proper authentication logic
    return true;
}

std::string LANSyncServer::sanitize_filename(const std::string& filename) {
    std::string safe = filename;
    size_t pos = safe.find_last_of("/\\");
    if (pos != std::string::npos) {
        safe = safe.substr(pos + 1);
    }
    for (char& c : safe) {
        if (c == '.' && safe == "..") return ""; 
        if (c < 32 || c == '<' || c == '>' || c == ':' || c == '"' || 
            c == '|' || c == '?' || c == '*' || c== ';') {
            c = '_';
        }
    }
    return safe.empty() ? "unnamed_file" : safe;
}

bool LANSyncServer::write_file_safely(const std::string& path, const std::string& content) {
    try {
        std::ofstream file(path, std::ios::binary);
        
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path << std::endl;
            std::cerr << "Error code: " << errno << std::endl;
            std::cerr << "Error description: " << strerror(errno) << std::endl;
            return false;
        }
    
        std::filesystem::path filePath(path);
        if (!std::filesystem::exists(filePath.parent_path())) {
            std::cerr << "Parent directory does not exist: " << filePath.parent_path() << std::endl;
            return false;
        }
    
        file.write(content.data(), content.size());
        
        if (!file.good()) {
            std::cerr << "Write operation failed" << std::endl;
            std::cerr << "Failbit set: " << file.fail() << std::endl;
            std::cerr << "Badbit set: " << file.bad() << std::endl;
            return false;
        }
    
        file.close();
        return true;
    
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error writing file: " << e.what() << std::endl;
        return false;
    }
}

void LANSyncServer::ensure_storage_directory() {
    std::filesystem::create_directories(storage_path);
}
