#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <mutex>
#include <winsock2.h>

class Server {
public:
    Server(const std::string& startPath, bool watchMode = false);
    
    void startWatching();
    void startServer();

private:
    std::string startPath;
    bool watchMode;
    std::unordered_map<std::string, std::filesystem::file_time_type> fileTimestamps;
    std::vector<SOCKET> sseClients; // Track SSE connections for hot reload
    std::mutex sseClientsMutex;

    void checkForChanges();
    void scanDirectory();
    void notifyClients(const std::string& message);
    void handleClient(SOCKET clientSocket);
    void handleSSE(SOCKET clientSocket);
    void serveFile(SOCKET clientSocket, const std::string& requestedPath);
    void serveHTMLWithHotReload(SOCKET clientSocket, const std::string& fullPath);
    std::string getContentType(const std::string& path);
};
