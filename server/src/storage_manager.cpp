#include "storage_manager.hpp"



bool StorageManager::enqueue_cache(const std::string& filename) {
    std::string source_path = cache_path + "/" + filename;
    size_t file_size = std::filesystem::file_size(source_path);
    if (!std::filesystem::exists(source_path)) {
        return false; 
    }
    {
    std::lock_guard<std::mutex> lock(queue_mutex);
    std::cout<<"enqueuing file: "<<filename<<std::endl;
    file_queue.push(filename);
    queue_size += file_size;
    cv.notify_one();
    }
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
        std::unique_lock<std::mutex> lock(queue_mutex);
        cv.wait(lock, [this]{ return !file_queue.empty(); });
        std::string filename = file_queue.front();
        file_queue.pop();
        queue_size -= std::filesystem::file_size(cache_path + "/" + filename);
        std::cout<<"dequeuing file: "<<filename<<std::endl;
        lock.unlock();
        move_file_to_storage(filename);
    }
}

bool StorageManager::move_file_to_storage(const std::string& filename) {
    std::string source_path = cache_path + "/" + filename;
    std::string dest_path = main_storage_path + "/" + filename;
    std::cout<<"moving file to storage: "<<filename<<std::endl;
    try {
        std::filesystem::copy(source_path, dest_path);
        std::filesystem::remove(source_path);
        db_manager->update_file_location(filename, "STORAGE");
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error moving file to storage: " << e.what() << std::endl;
        return false;
    }
}
