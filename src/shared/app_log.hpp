#pragma once

#include <QString>
#include <cstdio>

namespace ydisquette {

enum class LogLevel : int { Off = 0, Normal = 1, Verbose = 2 };

void setLogFile(FILE* f);
void setLogLevel(LogLevel level);
LogLevel getLogLevel();

void logToFile(const QString& msg);
void logToFile(const char* msg);

void log(LogLevel minLevel, const QString& msg);
void log(LogLevel minLevel, const char* msg);

}
