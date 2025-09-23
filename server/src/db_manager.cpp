#include "db_manager.hpp"
#include <iostream>

DBManager::DBManager(const std::string& db_path) : db(nullptr) {
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    } else {
        std::cout << "Opened database successfully" << std::endl;
        initialize_schema();
    }
}

DBManager::~DBManager() {
    if (db) {
        sqlite3_close(db);
    }
}

void DBManager::initialize_schema() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS files ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "filename TEXT NOT NULL UNIQUE,"
        "sha256_hash TEXT NOT NULL,"
        "size_bytes INTEGER NOT NULL,"
        "location TEXT NOT NULL DEFAULT 'CACHE',"
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_files_hash ON files (sha256_hash);";

    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql, 0, 0, &err_msg) != SQLITE_OK) {
        std::cerr << "SQL error: " << err_msg << std::endl;
        sqlite3_free(err_msg);
    } else {
        std::cout << "Schema initialized successfully." << std::endl;
    }
}


bool DBManager::add_file(const std::string& filename, const std::string& hash, long long size) {
    const char* sql = "INSERT INTO files (filename, sha256_hash, size_bytes, location) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, size);
    sqlite3_bind_text(stmt, 4, "CACHE", -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!success) {
        std::cerr << "Failed to add file: " << sqlite3_errmsg(db) << std::endl;
    }
    return success;
}

std::vector<FileRecord> DBManager::get_all_files() {
    std::vector<FileRecord> records;
    const char* sql = "SELECT id, filename, sha256_hash, size_bytes, location, created_at FROM files ORDER BY created_at DESC;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return records;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord rec;
        rec.id = sqlite3_column_int64(stmt, 0);
        rec.filename = (const char*)sqlite3_column_text(stmt, 1);
        rec.sha256_hash = (const char*)sqlite3_column_text(stmt, 2);
        rec.size_bytes = sqlite3_column_int64(stmt, 3);
        rec.location = (const char*)sqlite3_column_text(stmt, 4);
        rec.created_at = (const char*)sqlite3_column_text(stmt, 5);
        records.push_back(rec);
    }

    sqlite3_finalize(stmt);
    return records;
}


std::optional<FileRecord> DBManager::get_file_by_hash(const std::string& hash) {
    const char* sql = "SELECT id, filename, sha256_hash, size_bytes, location, created_at FROM files WHERE sha256_hash = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    std::optional<FileRecord> record;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return record;

    sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord rec;
        rec.id = sqlite3_column_int64(stmt, 0);
        rec.filename = (const char*)sqlite3_column_text(stmt, 1);
        rec.sha256_hash = (const char*)sqlite3_column_text(stmt, 2);
        rec.size_bytes = sqlite3_column_int64(stmt, 3);
        rec.location = (const char*)sqlite3_column_text(stmt, 4);
        rec.created_at = (const char*)sqlite3_column_text(stmt, 5);
        record = rec;
    }

    sqlite3_finalize(stmt);
    return record;
}

std::optional<FileRecord> DBManager::get_file_by_name(const std::string& filename) {
    const char* sql = "SELECT id, filename, sha256_hash, size_bytes, location, created_at FROM files WHERE filename = ? LIMIT 1;";
    sqlite3_stmt* stmt;
    std::optional<FileRecord> record;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return record;

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        FileRecord rec;
        rec.id = sqlite3_column_int64(stmt, 0);
        rec.filename = (const char*)sqlite3_column_text(stmt, 1);
        rec.sha256_hash = (const char*)sqlite3_column_text(stmt, 2);
        rec.size_bytes = sqlite3_column_int64(stmt, 3);
        rec.location = (const char*)sqlite3_column_text(stmt, 4);
        rec.created_at = (const char*)sqlite3_column_text(stmt, 5);
        record = rec;
    }

    sqlite3_finalize(stmt);
    return record;
}

bool DBManager::update_file_location(const std::string& filename, const std::string& new_location) {
    const char* sql = "UPDATE files SET location = ? WHERE filename = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, new_location.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, filename.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!success) {
        std::cerr << "Failed to update file location: " << sqlite3_errmsg(db) << std::endl;
    }
    return success;
}

bool DBManager::delete_file(const std::string& filename) {
    const char* sql = "DELETE FROM files WHERE filename = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_STATIC);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (!success) {
        std::cerr << "Failed to delete file: " << sqlite3_errmsg(db) << std::endl;
    }
    return success;
}