#include "server.hpp"
#include <cstdlib>

#define DEFAULT_SSD_CACHE "/mnt/ssd_cache"
#define DEFAULT_HDD_STORAGE "/mnt/hdd_storage"

int main() {
    const char* ssd_path_env = std::getenv("SSD_CACHE_PATH");
    const char* hdd_path_env = std::getenv("HDD_STORAGE_PATH");

    std::string ssd_path = ssd_path_env ? ssd_path_env : DEFAULT_SSD_CACHE;
    std::string hdd_path = hdd_path_env ? hdd_path_env : DEFAULT_HDD_STORAGE;



    LANSyncServer server(ssd_path,hdd_path, 50 * 1024 * 1024); 
    server.start_server("0.0.0.0", 8080);
    return 0;
}