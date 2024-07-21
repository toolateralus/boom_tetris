#pragma once
#include <string>

struct ScoreFile {
  size_t high_score = 0;
  static std::string getScoreFilePath();
  static void createDirectoryAndFile(const std::string &path);
  void read();
  void write();
};