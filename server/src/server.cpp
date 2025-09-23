#include "server.hpp"

void LANSyncServer::start_server(const std::string& host = "0.0.0.0", int port = 8080) {
    std::cout << "Starting LAN Drive server on " << host << ":" << port << std::endl;
    std::cout << "Storage path: " << ssd_cache_path << std::endl;
    server.listen(host, port);
    std::cerr << "Something wrong listening\n Error code: " << errno << std::endl;
    std::cerr << "Error description: " << strerror(errno) << std::endl;

}

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

    server.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream html("web/index.html");
        if (!html) {
            res.set_content("Frontend not found", "text/plain");
            res.status = 404;
            return;
        }
        std::stringstream buffer;
        buffer << html.rdbuf();
        res.set_content(buffer.str(), "text/html");
    });
    server.Get("/app.js", [](const httplib::Request&, httplib::Response& res) {
        std::ifstream js("web/app.js");
        if (!js) {
            res.set_content("Frontend script not found", "text/plain");
            res.status = 404;
            return;
        }

        std::stringstream buffer;
        buffer << js.rdbuf();
        res.set_content(buffer.str(), "application/javascript");
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

    server.Get("/delete/(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string filename = req.matches[1];
        handle_file_delete(filename, req, res);
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

    std::string hash = calculate_sha256(req.body);

    if (db_manager->get_file_by_hash(hash).has_value()) {
        res.status = 409;
        res.set_content("{\"error\": \"File already exists (same content)\"}", "application/json");
        return;
    }
    
    //could be optimised
    if (db_manager->get_file_by_name(filename).has_value()) {
        int version=1;
        while(db_manager->get_file_by_name(filename+"_("+std::to_string(version)+")").has_value()){
            version++;
        }
        std::cout<<"this is not an infinite loop right?"<<std::endl;
        
        filename.append("_("+std::to_string(version)+")");
    }

    std::string full_path = ssd_cache_path + "/" + filename;
    if (write_file_safely(filename,full_path, req.body)) {
        res.status = 200;
        db_manager->add_file(filename, hash, req.body.size());
        res.set_content("{\"message\": \"Upload successful\", \"filename\": \"" + filename + "\"}", 
                      "application/json");
    } else {
        res.status = 500;
        res.set_content("{\"error\": \"Failed to save file\"}", "application/json");
    }
}

void LANSyncServer::handle_file_download(const std::string& filename, const httplib::Request& req, httplib::Response& res) {
    
    std::string safe_filename = sanitize_filename(filename);
    std::optional<FileRecord> record;
    if(!(record=db_manager->get_file_by_name(safe_filename)).has_value()){
        res.status = 404;
        res.set_content("{\"error\": \"File not found \"}", "application/json");
        return;
    }

    std::string full_path = (record->location=="CACHE" ? ssd_cache_path : hdd_storage_path)+ "/" + safe_filename;
    
    std::ifstream file(full_path, std::ios::binary);
    if (!file) {
        res.status = 500;
        res.set_content("{\"error\": \"Cannot read file\"}", "application/json");
        return;
    }

    auto file_size = record->size_bytes;

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

// In server/src/server.cpp

void LANSyncServer::handle_list_files(const httplib::Request& req, httplib::Response& res) {
    auto files = db_manager->get_all_files();
    
    std::string json_response = "{\"files\": [";
    bool first = true;
    for (const auto& file : files) {
        if (!first) json_response += ",";
        json_response += "{";
        json_response += "\"name\": \"" + file.filename + "\",";
        json_response += "\"size\": " + std::to_string(file.size_bytes) + ",";
        
        // Note: C++20 has better time parsing, but this is a simple way
        // For a robust solution, you'd parse file.created_at
        json_response += "\"modified\": 0"; // Placeholder for timestamp

        json_response += "}";
        first = false;
    }
    
    json_response += "]}";
    res.set_content(json_response, "application/json");
}


void LANSyncServer::handle_file_delete(const std::string& filename, const httplib::Request& req, httplib::Response& res) {

    std::string safe_filename = sanitize_filename(filename);
    std::optional<FileRecord> record;
    if(!(record=db_manager->get_file_by_name(safe_filename)).has_value()){
        res.status = 404;
        res.set_content("{\"error\": \"File not found \"}", "application/json");
        return;
    }

    std::string full_path = (record->location=="CACHE" ? ssd_cache_path : hdd_storage_path)+ "/" + safe_filename;
    
    if (std::filesystem::remove(full_path)) {
        res.status = 200;
        res.set_content("{\"message\": \"File deleted\"}", "application/json");
        db_manager->delete_file(safe_filename);
    } else {
        res.status = 404;
        res.set_content("{\"error\": \"File not found\"}", "application/json");
    }
}

void LANSyncServer::handle_file_info(const std::string& filename, const httplib::Request& req, httplib::Response& res) {
    
    std::string safe_filename = sanitize_filename(filename);

    std::optional<FileRecord> record;
    if(!(record=db_manager->get_file_by_name(safe_filename)).has_value()){
        res.status = 404;
        res.set_content("{\"error\": \"File not found \"}", "application/json");
        return;
    }
    
    std::string json_response = "{";
    json_response += "\"name\": \"" + safe_filename + "\",";
    json_response += "\"size\": " + std::to_string(record->size_bytes) + ",";
    json_response += "\"modified\": " + record->created_at + ",";
    json_response += "}";
    
    res.set_content(json_response, "application/json");
    
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

bool LANSyncServer::write_file_safely(const std::string& filename,const std::string& path, const std::string& content) {
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
        storage_manager->enqueue_cache(filename);
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
    std::filesystem::create_directories(ssd_cache_path);
    std::filesystem::create_directories(hdd_storage_path);
}
