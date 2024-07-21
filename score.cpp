#include "score.hpp"
#include <filesystem>
#include <fstream>

#ifdef _WIN32
  #include "windows.h"
#endif
#include <iostream>


void ScoreFile::read() {
  std::string filename = getScoreFilePath();
  if (filename.empty()) {
    return;
  }
  std::ifstream file(filename);
  if (file.is_open()) {
    std::string s;
    file >> s;
    high_score = std::atoi(s.c_str());
    file.close();
  }
}
void ScoreFile::write() {
  std::string filename = getScoreFilePath();
  if (filename.empty()) {
    return;
  }
  std::ofstream file(filename);
  if (file.is_open()) {
    file << std::to_string(high_score);
    file.close();
  }
}
std::string ScoreFile::getScoreFilePath() {
    std::filesystem::path path;

    // Use std::filesystem to find the appropriate app data directory
    #ifdef _WIN32
    auto appDataPath = std::getenv("APPDATA");
    if (appDataPath != nullptr) {
        path = std::filesystem::path(appDataPath) / "boom_tetris" / "score";
    }
    #else
    auto homePath = std::getenv("HOME");
    if (homePath != nullptr) {
        path = std::filesystem::path(homePath) / ".config" / "boom_tetris" / "score";
    }
    #endif
    // Create the directory if it doesn't exist
    if (!std::filesystem::exists(path.parent_path())) {
        std::filesystem::create_directories(path.parent_path());
    }

    return path.string();
}

void ScoreFile::createDirectoryAndFile(const std::string &path) {
  try {
    std::filesystem::path dirPath = std::filesystem::path(path).parent_path();
    if (!std::filesystem::exists(dirPath)) {
      bool created = std::filesystem::create_directories(dirPath);
      if (created) {
        std::cout << "Directory created successfully: " << dirPath << std::endl;
      } else {
        std::cerr << "Failed to create directory: " << dirPath << std::endl;
        return;
      }
    }

    std::filesystem::path filePath = std::filesystem::path(path);
    if (!std::filesystem::exists(filePath)) {
      std::ofstream file(path);
      if (file) {
        std::cout << "File created successfully: " << filePath << std::endl;
      } else {
        std::cerr << "Failed to create file: " << filePath << std::endl;
      }
    }
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Filesystem error: " << e.what() << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "General error: " << e.what() << std::endl;
  }
}
