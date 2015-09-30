//
// Created by liubotao on 15/9/30.
//

#ifndef AGENT_LOGGING_H
#define AGENT_LOGGING_H


#include <stdio.h>
#include <pthread.h>
#include <string>
#include "MutexLock.h"
#include "macro_define.h"

typedef enum LogLevel {
    LL_DEBUG = 1,
    LL_TRACE = 2,
    LL_INFO = 3,
    LL_WARNING = 4,
    LL_ERROR = 5,
} LogLevel;

class Logging {
public:
    Logging() : level_(LL_INFO), fp_(NULL), isSync_(false), isAppend_(true), logFile_(), mutex_() { }

    ~Logging() {
        closeLogFile();
    }

    bool logInit(LogLevel logLevel, std::string &filename, bool append = true,
                 bool issync = false);

    bool log(LogLevel logLevel, const char *logformat, ...);

    LogLevel getLevel() const { return level_; }

    bool closeLogFile();

private:
    inline const char *logLevelToString(LogLevel logLevel);

    inline bool checkLevel(LogLevel logLevel);

    int premakestr(char *m_buffer, LogLevel logLevel);

    bool writeLog(char *_pbuffer, int len);


private:
    enum LogLevel level_;
    FILE *fp_;
    bool isSync_;
    bool isAppend_;
    std::string logFile_;
    MutexLock mutex_;

    static const int LOG_BUFFSIZE = 1024 * 1024 * 4;
    static const int SYS_BUFFSIZE = 1024 * 1024 * 8;
    static const size_t LOG_PATH_LEN = 250;
    static const int LOG_MODULE_NAME_LEN = 32;

    static __thread char buffer[LOG_BUFFSIZE];

private:
    Logging(const Logging &);

    Logging &operator=(const Logging &);
};

extern Logging WARN_W;
extern Logging INFO_W;


#endif //AGENT_LOGGING_H
