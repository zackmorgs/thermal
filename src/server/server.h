#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <vector>
#include <mutex>

// Linux socket headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

class Server {
public:
    Server(const std::string& startPath, bool watchMode = false, int port = 8080);
    
    void startWatching();
    void startServer();

private:
    std::string startPath;
    bool watchMode;
    int port;
    std::unordered_map<std::string, std::filesystem::file_time_type> fileTimestamps;
    std::vector<int> sseClients; // Track SSE connections for hot reload (using int instead of SOCKET)
    std::mutex sseClientsMutex;

    void checkForChanges();
    void scanDirectory();
    void notifyClients(const std::string& message);
    void handleClient(int clientSocket);
    void handleSSE(int clientSocket);
    void serveFile(int clientSocket, const std::string& requestedPath);
    void serveHTMLWithHotReload(int clientSocket, const std::string& fullPath);
    std::string getContentType(const std::string& path);
};
