#ifndef DB_MANAGER_HPP
#define DB_MANAGER_HPP
#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>

struct FileRecord {
    long long id;
    std::string filename;
    std::string sha256_hash;
    long long size_bytes;
    std::string location; 
    std::string created_at;
};

class DBManager {
private:
    sqlite3* db;
    void initialize_schema();
public:
    DBManager(const std::string& db_path);
    ~DBManager();

    bool add_file(const std::string& filename, const std::string& hash, long long size);
    std::optional<FileRecord> get_file_by_hash(const std::string& hash);
    std::optional<FileRecord> get_file_by_name(const std::string& filename);
    std::vector<FileRecord> get_all_files();
    bool update_file_location(const std::string& filename, const std::string& new_location);
    bool delete_file(const std::string& filename);
};

#endif 

   