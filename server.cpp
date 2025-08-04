#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")



namespace fs = std::filesystem;

class Server {
    public:
    Server(const std::string& startPath, bool watchMode = false)
    {
        this->startPath = startPath;
        if (startPath.empty()) {
            this->startPath = "./";
        }
        this->watchMode = watchMode;
        std::cout << "Server initialized with start path: " << this->startPath << std::endl;
        // Don't automatically start watching - let main() control this
    }
    
    void startWatching() {
        std::cout << "Watch mode is enabled" << std::endl;
        
        // Initial scan to populate fileTimestamps
        scanDirectory();
        
        while (true) {
            checkForChanges();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Check every second
        }
    }
    
    void startServer() {
        int port = 8080;
        
        std::cout << "Server started at path: " << startPath << std::endl;
        
        // Initialize Winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != 0) {
            std::cerr << "WSAStartup failed: " << iResult << std::endl;
            return;
        }
        
        // Create socket
        SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }
        
        // Setup address
        sockaddr_in service;
        service.sin_family = AF_INET;
        service.sin_addr.s_addr = INADDR_ANY;
        service.sin_port = htons(port); // Port 8080
        
        // Bind socket
        if (bind(listenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return;
        }
        
        // Listen for connections
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return;
        }
        
        std::cout << "Server is running on http://localhost:8080" << std::endl;
        std::cout << "Serving files from: " << startPath << std::endl;
        
        // Accept connections
        while (true) {
            SOCKET clientSocket = accept(listenSocket, NULL, NULL);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                continue;
            }
            
            // Handle client in separate thread
            std::thread([this, clientSocket]() {
                handleClient(clientSocket);
            }).detach();
        }
        
        closesocket(listenSocket);
        WSACleanup();
    }
    
    private:
    std::string startPath;
    bool watchMode;
    std::unordered_map<std::string, std::filesystem::file_time_type> fileTimestamps;
    std::vector<SOCKET> sseClients; // Track SSE connections for hot reload
    std::mutex sseClientsMutex;
    
    void checkForChanges() {
        try {
            // Check for modified or new files
            for (const auto& entry : fs::recursive_directory_iterator(startPath)) {
                if (entry.is_regular_file()) {
                    const std::string filePath = entry.path().string();
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
    
    void scanDirectory() {
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

    void notifyClients(const std::string& message) {
        std::lock_guard<std::mutex> lock(sseClientsMutex);
        std::string sseMessage = "data: " + message + "\n\n";
        
        auto it = sseClients.begin();
        while (it != sseClients.end()) {
            int result = send(*it, sseMessage.c_str(), sseMessage.length(), 0);
            if (result == SOCKET_ERROR) {
                // Client disconnected, remove from list
                closesocket(*it);
                it = sseClients.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    void handleClient(SOCKET clientSocket) {
        char buffer[4096];
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
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
        
        closesocket(clientSocket);
    }

    void handleSSE(SOCKET clientSocket) {
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
    
    void serveFile(SOCKET clientSocket, const std::string& requestedPath) {
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

    void serveHTMLWithHotReload(SOCKET clientSocket, const std::string& fullPath) {
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
    
    std::string getContentType(const std::string& path) {
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
};