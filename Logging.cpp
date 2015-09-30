//
// Created by liubotao on 15/9/30.
//

#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include "Logging.h"

using namespace std;

Logging WARN_W;
Logging INFO_W;

__thread char Logging::buffer[LOG_BUFFSIZE];

bool LOG_INIT(LogLevel logLevel, const char *moduleName, const char *logDir) {
    if (access(logDir, 0) == -1) {
        if (mkdir(logDir, S_IREAD | S_IWRITE) < 0) {
            cout << "ERROR: create log folder failed\n";
            exit(1);
        }
    }

    string localtion_info = logDir;
    localtion_info += "/";
    localtion_info += moduleName;
    localtion_info += ".log";
    cout << localtion_info << endl;
    INFO_W.logInit(logLevel, localtion_info);

    return true;
}

const char *Logging::logLevelToString(LogLevel logLevel) {

    switch (logLevel) {
        case LL_DEBUG:
            return "DEBUG";
        case LL_TRACE:
            return "TRACE";
        case LL_INFO:
            return "INFO";
        case LL_WARNING:
            return "WARN";
        case LL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

bool Logging::checkLevel(LogLevel logLevel) {
    return logLevel >= level_ ? true : false;
}

bool Logging::logInit(LogLevel logLevel, std::string &filename, bool append, bool issync) {
    MACRO_RET(NULL != fp_, false);
    level_ = logLevel;
    isAppend_ = append;
    isSync_ = issync;
    if (filename.length() >= Logging::LOG_PATH_LEN) {
        cout << "ERROR: the path of log file length is too long" << LOG_PATH_LEN << endl;
        exit(0);
    }

    logFile_ = filename;

    fp_ = fopen(logFile_.c_str(), append ? "a" : "w");
    if (fp_ == NULL) {
        cout << "ERROR: cannot open log file, file location is" << logFile_ << endl;
        exit(0);
    }

    setvbuf(fp_, (char *) NULL, _IOLBF, 0);
    cout << "INFO now all the running-information are going to the file [" << logFile_ << "]"<< endl;
    return true;
}

int Logging::premakestr(char *m_buffer, LogLevel logLevel) {
    time_t now;
    now = time(&now);
    struct tm vtm;
    localtime_r(&now, &vtm);
    return snprintf(m_buffer, LOG_BUFFSIZE, "[%s] %02d-%02d %02d:%02d:%02d ",
                    logLevelToString(logLevel), vtm.tm_mon + 1, vtm.tm_mday, vtm.tm_hour,
                    vtm.tm_min, vtm.tm_sec);
}

bool Logging::log(LogLevel logLevel, const char *logformat, ...) {

    MACRO_RET(!checkLevel(logLevel), false);

    char *star = buffer;
    int prestrlen = premakestr(star, logLevel);
    star += prestrlen;

    va_list args;
    va_start(args, logformat);
    int size = vsnprintf(star, LOG_BUFFSIZE - prestrlen, logformat, args);
    va_end(args);

    if (NULL == fp_) {
        fprintf(stderr, "%s", buffer);
    } else {
        writeLog(buffer, prestrlen + size);
    }

    return true;
}

bool Logging::writeLog(char *buf, int len) {

    if (access(logFile_.c_str(), W_OK) != 0) {
        MutexLockGuard lock(mutex);
        if (access(logFile_.c_str(), W_OK) != 0) {
            closeLogFile();
            logInit(level_, logFile_, isAppend_, isSync_);
        }
    }

    if (fwrite(buf, len, 1, fp_) == 1) {
        if (isSync_) {
            fflush(fp_);
        }
        *buf = '\0';
    } else {
        int x = errno;
        cout << "Error Failed to write to logfile, errno:" << strerror(x) << endl;
        return false;
    }

    return true;
}


bool Logging::closeLogFile() {
    if (fp_ == NULL) {
        return false;
    }

    fflush(fp_);
    fclose(fp_);
    fp_ = NULL;
    return true;
}

int main() {
    LOG_INIT(LogLevel::LL_DEBUG, "monitor", "/data/logs");
    LOG_ERROR("agent server start failure: load config error!");

}