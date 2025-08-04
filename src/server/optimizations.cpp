// Key optimizations for your thermal server:

// 1. FILE WATCHING OPTIMIZATIONS
// ===============================

void Server::checkForChanges() {
    try {
        std::vector<std::string> changedFiles;
        
        // Use iterative approach instead of recursive for better control
        std::filesystem::directory_iterator dirIter(startPath);
        
        for (const auto& entry : dirIter) {
            if (entry.is_directory() && shouldIgnoreDirectory(entry.path())) {
                continue; // Skip ignored directories like .git, node_modules
            }
            
            if (entry.is_regular_file() && !shouldIgnoreFile(entry.path())) {
                const std::string filePath = entry.path().string();
                const auto lastWriteTime = entry.last_write_time();
                
                auto it = fileTimestamps.find(filePath);
                if (it == fileTimestamps.end()) {
                    // New file
                    fileTimestamps[filePath] = lastWriteTime;
                    changedFiles.push_back(filePath);
                } else if (it->second != lastWriteTime) {
                    // Modified file
                    it->second = lastWriteTime;
                    changedFiles.push_back(filePath);
                    
                    // Invalidate cache for changed file
                    std::lock_guard<std::mutex> lock(fileCacheMutex);
                    fileCache.erase(filePath);
                }
            }
        }
        
        // Batch notify clients (more efficient than per-file notifications)
        if (!changedFiles.empty() && watchMode) {
            notifyClients("reload");
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error checking for changes: " << e.what() << std::endl;
    }
}

// 2. SMART FILE FILTERING
// =======================

bool Server::shouldIgnoreFile(const std::filesystem::path& path) {
    const auto extension = path.extension().string();
    
    // Ignore temporary files, logs, etc.
    if (ignoredExtensions.find(extension) != ignoredExtensions.end()) {
        return true;
    }
    
    // Ignore hidden files
    const auto filename = path.filename().string();
    if (filename.front() == '.') {
        return true;
    }
    
    // Ignore very large files (>10MB)
    try {
        if (std::filesystem::file_size(path) > 10 * 1024 * 1024) {
            return true;
        }
    } catch (...) {
        return true; // If can't get size, ignore
    }
    
    return false;
}

bool Server::shouldIgnoreDirectory(const std::filesystem::path& path) {
    const auto dirname = path.filename().string();
    return ignoredDirectories.find(dirname) != ignoredDirectories.end();
}

// 3. FILE CACHING SYSTEM
// ======================

std::string Server::getCachedFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(fileCacheMutex);
    
    auto it = fileCache.find(path);
    if (it != fileCache.end()) {
        // Check if cache is still valid
        try {
            auto currentModTime = std::filesystem::last_write_time(path);
            if (it->second.lastModified == currentModTime) {
                return it->second.content; // Cache hit!
            } else {
                fileCache.erase(it); // Cache miss, remove stale entry
            }
        } catch (...) {
            fileCache.erase(it); // File doesn't exist, remove from cache
        }
    }
    
    return ""; // Cache miss
}

void Server::cacheFile(const std::string& path, const std::string& content, 
                       const std::string& contentType) {
    if (content.size() > MAX_FILE_SIZE_TO_CACHE) {
        return; // Don't cache large files
    }
    
    std::lock_guard<std::mutex> lock(fileCacheMutex);
    
    // Implement LRU eviction if cache is full
    if (fileCache.size() >= MAX_CACHE_SIZE) {
        // Simple eviction: remove first entry (could be improved with proper LRU)
        fileCache.erase(fileCache.begin());
    }
    
    try {
        CacheEntry entry;
        entry.content = content;
        entry.contentType = contentType;
        entry.lastModified = std::filesystem::last_write_time(path);
        
        fileCache[path] = std::move(entry);
    } catch (...) {
        // Ignore cache errors
    }
}

// 4. CONNECTION MANAGEMENT OPTIMIZATIONS
// ======================================

void Server::cleanupDisconnectedClients() {
    std::lock_guard<std::mutex> lock(sseClientsMutex);
    
    auto it = sseClients.begin();
    while (it != sseClients.end()) {
        // Test connection with a non-blocking send
        ssize_t result = send(*it, "", 0, MSG_NOSIGNAL);
        if (result < 0) {
            close(*it);
            it = sseClients.erase(it);
        } else {
            ++it;
        }
    }
}

// 5. REDUCED POLLING FREQUENCY
// ============================

void Server::startWatching() {
    std::cout << "Watch mode is enabled" << std::endl;
    
    scanDirectory();
    
    auto lastCleanup = std::chrono::steady_clock::now();
    
    while (!shouldStop.load()) {
        checkForChanges();
        
        // Clean up disconnected clients every 30 seconds
        auto now = std::chrono::steady_clock::now();
        if (now - lastCleanup > std::chrono::seconds(30)) {
            cleanupDisconnectedClients();
            lastCleanup = now;
        }
        
        // Adaptive polling: shorter interval when files are changing
        auto sleepDuration = fileTimestamps.empty() ? 
            std::chrono::milliseconds(2000) :  // 2s when no files
            std::chrono::milliseconds(500);    // 500ms when monitoring files
            
        std::this_thread::sleep_for(sleepDuration);
    }
}

// 6. MEMORY OPTIMIZATIONS
// =======================

// Use string_view for read-only string operations
std::string_view getFileExtension(std::string_view path) {
    auto dotPos = path.find_last_of('.');
    return dotPos != std::string_view::npos ? 
           path.substr(dotPos) : std::string_view{};
}

// Use move semantics for large strings
void Server::serveFileOptimized(int clientSocket, const std::string& requestedPath) {
    std::string fullPath = startPath + "/" + requestedPath;
    
    // Check cache first
    std::string cachedContent = getCachedFile(fullPath);
    if (!cachedContent.empty()) {
        // Serve from cache - much faster!
        std::string contentType = getContentType(requestedPath);
        std::string headers = 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: " + contentType + "\r\n"
            "Content-Length: " + std::to_string(cachedContent.length()) + "\r\n"
            "\r\n";
        
        send(clientSocket, headers.c_str(), headers.length(), 0);
        send(clientSocket, cachedContent.c_str(), cachedContent.length(), 0);
        return;
    }
    
    // Read and cache file
    std::ifstream file(fullPath, std::ios::binary);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)), 
                           std::istreambuf_iterator<char>());
        
        std::string contentType = getContentType(requestedPath);
        cacheFile(fullPath, content, contentType);
        
        // Serve content...
    }
}

// 7. NETWORK OPTIMIZATIONS
// ========================

// Use TCP_NODELAY for lower latency
void Server::optimizeSocket(int sock) {
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    
    // Set larger buffer sizes
    int bufferSize = 64 * 1024; // 64KB
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufferSize, sizeof(int));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufferSize, sizeof(int));
}

// Use vectored I/O for sending headers + content efficiently
void Server::sendResponse(int sock, const std::string& headers, const std::string& body) {
    std::vector<char> response;
    response.reserve(headers.size() + body.size());
    
    response.insert(response.end(), headers.begin(), headers.end());
    response.insert(response.end(), body.begin(), body.end());
    
    send(sock, response.data(), response.size(), 0);
}
