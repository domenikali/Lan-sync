#include "server.hpp"

#define SSD_CACHE "/mnt/ssd_cache"
#define HDD_STORAGE "/mnt/hdd_storage"

int main() {
    LANSyncServer server(SSD_CACHE,HDD_STORAGE, 50 * 1024 * 1024); 
    server.start_server("0.0.0.0", 8080);
    return 0;
}