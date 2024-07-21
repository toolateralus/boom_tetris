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
  std::string path;
#ifdef _WIN32
  wchar_t wBuffer[MAX_PATH];
  if (GetEnvironmentVariableW(L"APPDATA", wBuffer, MAX_PATH)) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wBuffer, -1, NULL, 0, NULL, NULL);
    std::string narrowPath(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wBuffer, -1, &narrowPath[0], len, NULL, NULL);
    path = narrowPath + "\\boom_tetris\\score";
  }
#else
  const char *home = getenv("HOME");
  if (home != nullptr) {
    path = std::string(home) + "/.config/boom_tetris/score";
  }
#endif
  createDirectoryAndFile(path);
  return path;
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
