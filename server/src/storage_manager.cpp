#include "storage_manager.hpp"



bool StorageManager::enqueue_cache(const std::string& filename) {
    queue_mutex.lock();
    if (file_queue.size() >= queue_size) {
        return false; // Queue is full
    }
    file_queue.push(filename);
    queue_size += std::filesystem::file_size(cache_path + "/" + filename);
    queue_mutex.unlock();
    return true;
}

bool StorageManager::move_file_to_cache(const std::string& filename){
    std::string source_path = main_storage_path + "/" + filename;
    std::string dest_path = cache_path + "/" + filename;

    if (!std::filesystem::exists(source_path)) {
        return false; // Source file does not exist
    }

    try {
        std::filesystem::rename(source_path, dest_path);
        file_queue.push(filename);
        queue_size += std::filesystem::file_size(dest_path);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error moving file to cache: " << e.what() << std::endl;
        return false;
    }
}

void StorageManager::worker_thread() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); 
        queue_mutex.lock();
        if (!file_queue.empty()) {
            std::string filename = file_queue.front();
            file_queue.pop();
            queue_size -= std::filesystem::file_size(cache_path + "/" + filename);
            move_file_to_storage(filename);
        }
        queue_mutex.unlock();
        std::lock_guard<std::mutex> unlock(queue_mutex);
    }
}

bool StorageManager::move_file_to_storage(const std::string& filename) {
    std::string source_path = cache_path + "/" + filename;
    std::string dest_path = main_storage_path + "/" + filename;

    if (!std::filesystem::exists(source_path)) {
        return false; // Source file does not exist
    }

    try {
        std::filesystem::rename(source_path, dest_path);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error moving file to storage: " << e.what() << std::endl;
        return false;
    }
}
