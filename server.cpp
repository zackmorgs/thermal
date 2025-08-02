#include <iostream>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

class Server {
public:
    Server(const std::string& startPath, bool watchMode = false) 
        : startPath(startPath), watchMode(watchMode) {
        if (startPath.empty()) {
            this->startPath = "./";
        }
        this->watchMode = watchMode;
        std::cout << "Server initialized with start path: " << this->startPath << std::endl;
        if (watchMode) {
            this->startWatching();
        }
    }

    void startWatching() {
        if (!watchMode) {
            std::cout << "Watch mode is not enabled" << std::endl;
            return;
        } else {
            std::cout << "Watch mode is enabled" << std::endl;
            // scanDirectory();
        }
        
        while (true) {
            checkForChanges();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Check every second
        }
    }

private:
    std::string startPath;
    bool watchMode;
    std::unordered_map<std::string, std::filesystem::file_time_type> fileTimestamps;

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
                        // onFileChanged(filePath, "created");
                    } else if (it->second != lastWriteTime) {
                        // Modified file
                        std::cout << "File modified: " << filePath << std::endl;
                        it->second = lastWriteTime;
                        // onFileChanged(filePath, "modified");
                    }
                }
            }

            // Check for deleted files
            auto it = fileTimestamps.begin();
            while (it != fileTimestamps.end()) {
                if (!fs::exists(it->first)) {
                    std::cout << "File deleted: " << it->first << std::endl;
                    //onFileChanged(it->first, "deleted");
                    it = fileTimestamps.erase(it);
                } else {
                    ++it;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error checking for changes: " << e.what() << std::endl;
        }
    }
};