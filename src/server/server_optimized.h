#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class Server {
public:
    Server(const std::string& startPath, bool watchMode = false);
    
    void startWatching();
    void startServer();
    void shutdown();

private:
    std::string startPath;
    std::atomic<bool> watchMode;
    std::atomic<bool> shouldStop{false};
    
    // File watching optimizations
    std::unordered_map<std::string, std::filesystem::file_time_type> fileTimestamps;
    std::unordered_set<std::string> ignoredExtensions{".tmp", ".swp", ".log", ".lock"};
    std::unordered_set<std::string> ignoredDirectories{".git", ".vs", "node_modules", "build"};
    
    // Connection management
    std::vector<int> sseClients; // Using int instead of SOCKET
    std::mutex sseClientsMutex;
    
    // Thread pool for handling requests
    static constexpr size_t THREAD_POOL_SIZE = 4;
    
    // Caching
    struct CacheEntry {
        std::string content;
        std::string contentType;
        std::filesystem::file_time_type lastModified;
    };
    std::unordered_map<std::string, CacheEntry> fileCache;
    std::mutex fileCacheMutex;
    static constexpr size_t MAX_CACHE_SIZE = 100;
    static constexpr size_t MAX_FILE_SIZE_TO_CACHE = 1024 * 1024; // 1MB

    void checkForChanges();
    void scanDirectory();
    void notifyClients(const std::string& message);
    void handleClient(int clientSocket);
    void handleSSE(int clientSocket);
    void serveFile(int clientSocket, const std::string& requestedPath);
    void serveHTMLWithHotReload(int clientSocket, const std::string& fullPath);
    std::string getContentType(const std::string& path);
    
    // Optimization methods
    bool shouldIgnoreFile(const std::filesystem::path& path);
    bool shouldIgnoreDirectory(const std::filesystem::path& path);
    void cleanupDisconnectedClients();
    std::string getCachedFile(const std::string& path);
    void cacheFile(const std::string& path, const std::string& content, const std::string& contentType);
};
