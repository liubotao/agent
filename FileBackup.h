//
// Created by liubotao on 15/9/30.
//

#ifndef AGENT_FILEBACKUP_H
#define AGENT_FILEBACKUP_H


#include "MD5.h"

void delDirIfExist(const char *path);


class FileBackup {

public:
    FileBackup();

    ~FileBackup();

    void setSavePath(string savePath);

    bool saveNewFile(char *buf, int bufSize);

    bool saveNewConfig(char *buf, int bufSize);

    bool saveFinished();

    bool zipEtcDir();

    bool backupRecvFile();

    bool backupOldFile();

    bool checkFileMd5(string srcMd5);

    bool saveFileToPath();

    bool unzipFile();

    bool mvNewConf2Etc();

private:
    FILE *fd_;
    string fileName;
    string savePath;
};


#endif //AGENT_FILEBACKUP_H
