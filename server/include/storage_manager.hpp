#ifndef STORAGE_MANAGER_HPP
#define STORAGE_MANAGER_HPP

#include <thread>
#include <mutex>
#include <string>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <queue>
#include <condition_variable>


class StorageManager{
    private: 
        std::string main_storage_path;
        std::string cache_path;
        std::queue<std::string> file_queue;
        uint64_t storage_limit;
        uint64_t queue_size;
        std::mutex queue_mutex;
        std::condition_variable cv;
    public:
        StorageManager(const std::string& storage_path, const std::string& cache_dir)
            : main_storage_path(storage_path), cache_path(cache_dir) {
            ensure_storage_directory();
            std::thread t (&StorageManager::worker_thread, this);
            t.detach();
        }

        void ensure_storage_directory() {
            std::filesystem::create_directories(main_storage_path);
            std::filesystem::create_directories(cache_path);
        }

        void worker_thread();

        bool enqueue_cache(const std::string& filename);

        bool move_file_to_cache(const std::string& filename);
        
        bool move_file_to_storage(const std::string& filename);

        std::queue<std::string> get_file_queue() {
            return file_queue;
        }

        int get_queue_size() {
            return queue_size;
        }
        

};


#endif