#include "Logger.h"

void Logger::LogInfo(const std::string &log, const std::string &prefix,
                     const std::string_view &color) {
  auto time = Utils::time().str();
  auto indentation = time.size() + prefix.size() + 2;
  auto logStr = formatMessage(log, false, indentation).str();
  std::cout << "\x1b[36m" << time << endFormat << "\x1b[32m " << prefix
            << endFormat << " " << color << logStr << endFormat << "\n";
}

void Logger::LogDiag(const std::string &log, const std::string &prefix,
                     const std::string_view &color) {
  if (severity > Info) {
    auto time = Utils::time().str();
    auto indentation = time.size() + prefix.size() + 2;
    auto logStr = formatMessage(log, false, indentation).str();
    std::cout << "\x1b[36m" << time << endFormat << "\x1b[32m " << prefix
              << endFormat << " " << color << logStr << endFormat << "\n";
  }
}

void Logger::LogTrace(const std::string &log, const std::string &prefix,
                      const std::string_view &color) {
  if (severity == Trace) {
    auto time = Utils::time().str();
    auto indentation = time.size() + prefix.size() + 2;
    auto logStr = formatMessage(log, false, indentation).str();
    std::cout << "\x1b[36m" << time << endFormat << "\x1b[32m " << prefix
              << endFormat << " " << color << logStr << endFormat << "\n";
  }
}

void Logger::LogError(const std::string &log, const std::string &prefix) {
  auto time = Utils::time().str();
  auto indentation = time.size() + prefix.size() + 2;
  auto logStr = formatMessage(log, false, indentation).str();
  std::cout << "\x1b[36m" << time << endFormat << "\x1b[32m " << prefix
            << endFormat << " "
            << "\x1b[31m" << logStr << endFormat << "\n";
}

std::ostringstream Logger::formatMessage(const std::string &messageStr,
                                         bool addDoubleSpace, int indentation,
                                         int lineLimit) {
  lineLimit -= indentation;
  size_t startPos = 0, length = messageStr.length();

  std::ostringstream formattedMsg;
  while (startPos < length) {
    std::string line = messageStr.substr(startPos, lineLimit);
    if (startPos + lineLimit < length) {
      // Find the last space in the line to break it at word boundaries
      size_t lastSpace = line.find_last_of(" ");
      if (lastSpace != std::string::npos) {
        line = line.substr(0, lastSpace);
        startPos += lastSpace + 1; // Skip the space
      } else {
        startPos += line.length();
      }
    } else {
      startPos += line.length();
    }
    formattedMsg << line;
    if ((startPos) < length)
      formattedMsg << "\n" << std::string(indentation, ' ');
  }

  if (addDoubleSpace) {
    formattedMsg << "\n";
  }
  return formattedMsg;
}