#include "shared/app_log.hpp"
#include <QDateTime>
#include <QMutex>
#include <cstdio>

namespace ydisquette {

static FILE* g_log = nullptr;
static QMutex g_logMutex;
static LogLevel g_logLevel = LogLevel::Normal;

void setLogFile(FILE* f) {
    g_log = f;
}

void setLogLevel(LogLevel level) {
    g_logLevel = level;
}

LogLevel getLogLevel() {
    return g_logLevel;
}

static void writeLog(const char* msg) {
    if (!msg || !g_log) return;
    QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    fprintf(g_log, "[%s] %s\n", stamp.toUtf8().constData(), msg);
    fflush(g_log);
}

void logToFile(const QString& msg) {
    QMutexLocker lock(&g_logMutex);
    writeLog(msg.toUtf8().constData());
}

void logToFile(const char* msg) {
    if (!msg) return;
    QMutexLocker lock(&g_logMutex);
    writeLog(msg);
}

void log(LogLevel minLevel, const QString& msg) {
    if (static_cast<int>(g_logLevel) < static_cast<int>(minLevel)) return;
    QMutexLocker lock(&g_logMutex);
    writeLog(msg.toUtf8().constData());
}

void log(LogLevel minLevel, const char* msg) {
    if (static_cast<int>(g_logLevel) < static_cast<int>(minLevel)) return;
    QMutexLocker lock(&g_logMutex);
    writeLog(msg);
}

}
