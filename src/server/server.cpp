#include "server.h"
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace fs = std::filesystem;

// contructor, initializes startPath and watchMode
Server::Server(const std::string& startPath, bool watchMode, int port) 
    : startPath(startPath), watchMode(watchMode), port(port) {
    if (this->startPath.empty()) {
        this->startPath = "./";
    }
    std::cout << "Server initialized with start path: " << this->startPath << std::endl;
    std::cout << "Port configured: " << this->port << std::endl;
}

// file change detection method
void Server::startWatching() {
    std::cout << "Watch mode is enabled" << std::endl;
    
    // Initial scan to populate fileTimestamps
    scanDirectory();
    
    while (true) {
        checkForChanges();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // Reduced from 1000ms to 1500ms
    }
}

// start server method
void Server::startServer() {
    std::cout << "Server started at path: " << startPath << std::endl;
    
    // Create socket
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options: " << strerror(errno) << std::endl;
        close(listenSocket);
        return;
    }
    
    // Setup address
    sockaddr_in service;
    memset(&service, 0, sizeof(service));
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(port); // Use configurable port
    
    // Bind socket
    if (bind(listenSocket, (struct sockaddr*)&service, sizeof(service)) < 0) {
        std::cerr << "Bind failed on port " << port << ": " << strerror(errno) << std::endl;
        close(listenSocket);
        return;
    }
    
    // Listen for connections
    if (listen(listenSocket, SOMAXCONN) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(listenSocket);
        return;
    }
    
    std::cout << "Server is running on http://localhost:" << port << std::endl;
    std::cout << "Serving files from: " << startPath << std::endl; 
    
    // Try to open browser (Ubuntu-compatible)
    std::string browserStart = "xdg-open http://localhost:" + std::to_string(port) + " 2>/dev/null &";  
    system(browserStart.c_str());

    // Accept connections
    while (true) {
        int clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket < 0) {
            std::cerr << "Accept failed: " << strerror(errno) << std::endl;
            continue;
        }
        
        // Handle client in separate thread
        std::thread([this, clientSocket]() {
            handleClient(clientSocket);
        }).detach();
    }
    
    close(listenSocket);
}

void Server::checkForChanges() {
    try {
        // Check for modified or new files
        for (const auto& entry : fs::recursive_directory_iterator(startPath)) {
            if (entry.is_regular_file()) {
                const std::string filePath = entry.path().string();
                
                // Quick optimization: Skip common temp files
                const auto filename = entry.path().filename().string();
                if (filename.starts_with(".") || 
                    filename.ends_with(".tmp") || 
                    filename.ends_with(".swp") ||
                    filename.ends_with(".log")) {
                    continue; // Skip temp/hidden files
                }
                
                const auto lastWriteTime = entry.last_write_time();
                
                auto it = fileTimestamps.find(filePath);
                if (it == fileTimestamps.end()) {
                    // New file
                    std::cout << "New file detected: " << filePath << std::endl;
                    fileTimestamps[filePath] = lastWriteTime;
                    if (watchMode) {
                        notifyClients("reload");
                    }
                } else if (it->second != lastWriteTime) {
                    // Modified file
                    std::cout << "File modified: " << filePath << std::endl;
                    it->second = lastWriteTime;
                    if (watchMode) {
                        notifyClients("reload");
                    }
                }
            }
        }
        
        // Check for deleted files
        auto it = fileTimestamps.begin();
        while (it != fileTimestamps.end()) {
            if (!fs::exists(it->first)) {
                std::cout << "File deleted: " << it->first << std::endl;
                if (watchMode) {
                    notifyClients("reload");
                }
                it = fileTimestamps.erase(it);
            } else {
                ++it;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error checking for changes: " << e.what() << std::endl;
    }
}

void Server::scanDirectory() {
    std::cout << "Scanning directory for initial file state..." << std::endl;
    try {
        for (const auto& entry : fs::recursive_directory_iterator(startPath)) {
            if (entry.is_regular_file()) {
                fileTimestamps[entry.path().string()] = entry.last_write_time();
            }
        }
        std::cout << "Found " << fileTimestamps.size() << " files to monitor" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
}

void Server::notifyClients(const std::string& message) {
    std::lock_guard<std::mutex> lock(sseClientsMutex);
    std::string sseMessage = "data: " + message + "\n\n";
    
    auto it = sseClients.begin();
    while (it != sseClients.end()) {
        ssize_t result = send(*it, sseMessage.c_str(), sseMessage.length(), 0);
        if (result < 0) {
            // Client disconnected, remove from list
            close(*it);
            it = sseClients.erase(it);
        } else {
            ++it;
        }
    }
}

void Server::handleClient(int clientSocket) {
    char buffer[4096];
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        std::string request(buffer);
        
        // Parse HTTP request
        std::istringstream iss(request);
        std::string method, path, version;
        iss >> method >> path >> version;
        
        std::cout << "Request: " << method << " " << path << std::endl;
        
        // Handle SSE endpoint for hot reload
        if (path == "/sse") {
            handleSSE(clientSocket);
            return; // Don't close socket, keep for SSE
        }
        
        // Remove leading slash and decode path
        if (path == "/") {
            path = "/index.html";
        }
        path = path.substr(1); // Remove leading slash
        
        // Serve file
        serveFile(clientSocket, path);
    }
    
    close(clientSocket);
}

void Server::handleSSE(int clientSocket) {
    // Send SSE headers
    std::string headers = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n";
    send(clientSocket, headers.c_str(), headers.length(), 0);
    
    // Add client to SSE list
    {
        std::lock_guard<std::mutex> lock(sseClientsMutex);
        sseClients.push_back(clientSocket);
    }
    
    std::cout << "SSE client connected for hot reload" << std::endl;
    
    // Keep connection alive (it will be closed when client disconnects or on error)
}

void Server::serveFile(int clientSocket, const std::string& requestedPath) {
    std::string fullPath = startPath + "/" + requestedPath;
    
    // Check if file exists and is within served directory
    if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
        // File not found - serve 404
        std::string response = 
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 44\r\n"
        "\r\n"
        "<html><body><h1>404 Not Found</h1></body></html>";
        send(clientSocket, response.c_str(), response.length(), 0);
        return;
    }
    
    // Read file content
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        // Error reading file
        std::string response = 
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 58\r\n"
        "\r\n"
        "<html><body><h1>500 Internal Server Error</h1></body></html>";
        send(clientSocket, response.c_str(), response.length(), 0);
        return;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Determine content type
    std::string contentType = getContentType(requestedPath);
    
    // For HTML files, inject hot reload script if in watch mode
    if (watchMode && (contentType == "text/html")) {
        serveHTMLWithHotReload(clientSocket, fullPath);
        return;
    }
    
    // Send HTTP headers
    std::string headers = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: " + contentType + "\r\n"
    "Content-Length: " + std::to_string(fileSize) + "\r\n"
    "\r\n";
    send(clientSocket, headers.c_str(), headers.length(), 0);
    
    // Send file content
    char fileBuffer[4096];
    while (file.read(fileBuffer, sizeof(fileBuffer)) || file.gcount() > 0) {
        send(clientSocket, fileBuffer, file.gcount(), 0);
    }
    
    file.close();
}

void Server::serveHTMLWithHotReload(int clientSocket, const std::string& fullPath) {
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        std::string response = 
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 58\r\n"
            "\r\n"
            "<html><body><h1>500 Internal Server Error</h1></body></html>";
        send(clientSocket, response.c_str(), response.length(), 0);
        return;
    }
    
    // Read entire file
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Hot reload script
    std::string hotReloadScript = R"(
<script>
(function() {
    console.log('🔥 Hot reload enabled');
    const eventSource = new EventSource('/sse');
    eventSource.onmessage = function(event) {
        if (event.data === 'reload') {
            console.log('🔄 Reloading page due to file change');
            window.location.reload();
        }
    };
    eventSource.onerror = function(event) {
        console.log('❌ Hot reload connection lost');
    };
})();
</script>
)";
    
    // Inject script before closing </body> or </html> tag
    size_t insertPos = content.find("</body>");
    if (insertPos == std::string::npos) {
        insertPos = content.find("</html>");
    }
    
    if (insertPos != std::string::npos) {
        content.insert(insertPos, hotReloadScript);
    } else {
        // If no closing tags found, append at the end
        content += hotReloadScript;
    }
    
    // Send response
    std::string headers = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(content.length()) + "\r\n"
        "\r\n";
    send(clientSocket, headers.c_str(), headers.length(), 0);
    send(clientSocket, content.c_str(), content.length(), 0);
}

std::string Server::getContentType(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string extension = path.substr(dotPos + 1);
    
    if (extension == "html" || extension == "htm") return "text/html";
    if (extension == "css") return "text/css";
    if (extension == "js") return "application/javascript";
    if (extension == "json") return "application/json";
    if (extension == "png") return "image/png";
    if (extension == "jpg" || extension == "jpeg") return "image/jpeg";
    if (extension == "gif") return "image/gif";
    if (extension == "txt") return "text/plain";
    
    return "application/octet-stream";
}
