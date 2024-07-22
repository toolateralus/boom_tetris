#pragma once
#include <chrono>
#include <string>

struct ScoreFile {
  size_t high_score = 0;
  std::chrono::milliseconds fortyLinesPb = {};
  
  static std::string getScoreFilePath();
  static void createDirectoryAndFile(const std::string &path);
  
  
  
  void read();
  void write();
};