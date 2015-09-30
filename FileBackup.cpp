//
// Created by liubotao on 15/9/30.
//

#include <dirent.h>
#include <unistd.h>
#include "FileBackup.h"
#include "Util.h"
#include "macro_define.h"
#include "Logging.h"


/*extern int maxFileBackupNumber;
extern string etcPath;
extern string etcBackupPath;
extern string recvConfigPath;*/

int maxFileBackupNumber;
string etcPath;
string etcBackupPath;
string recvConfigPath;

void delDirIfExist(const char *path) {
    DIR *dp = NULL;
    DIR *dpin = NULL;
    char pathname[256] = {0};
    struct dirent *dirp;
    dp = opendir(path);
    if (dp == NULL) {
        return;
    }

    while ((dirp = readdir(dp)) != NULL) {
        if (strcmp(dirp->d_name, "..") == 0 || strcmp(dirp->d_name, ".") == 0) {
            continue;
        }
        strcpy(pathname, path);
        strcat(pathname, "/");
        strcat(pathname, dirp->d_name);
        dpin = opendir(pathname);
        if (dpin != NULL) {
            delDirIfExist(pathname);
        } else {
            remove(pathname);
        }

        strcpy(pathname, "");
        closedir(dpin);
        dpin = NULL;
    }

    rmdir(path);
    closedir(dp);
    dirp = NULL;
}


FileBackup::FileBackup() : fd_(NULL) {

}

FileBackup::~FileBackup() {
    if (fd_ != NULL) {
        fclose(fd_);
        fd_ = NULL;
    }
}

void FileBackup::setSavePath(string savePath) {
    this->savePath = savePath;
    this->fileName = getFileName(savePath);
}

bool FileBackup::saveNewFile(char *buf, int bufSize) {
    if (fd_ == NULL) {
        string filePath = recvConfigPath + "/" + this->fileName + ".tmp";
        fd_ = fopen(filePath.c_str(), "w");
        if (!fd_) {
            int e = errno;
            LOG_ERROR("create temp file %s failed because of %s", filePath.c_str(), strerror);
            return false;
        }
    }

    if (bufSize > 0) {
        while (bufSize > 0) {
            size_t w = fwrite((void *) buf, 1, bufSize, fd_);
            bufSize -= w;
            buf += w;
        }
    } else {
        if (fclose(fd_) != 0) {
            int e = errno;
            LOG_INFO("fclose failed because of %s", strerror(e));
            return false;
        }
        fd_ = NULL;
    }
    return true;
}

bool FileBackup::saveNewConfig(char *buf, int bufSize) {

    if (fd_ == NULL) {
        string fn = recvConfigPath + "/" + "named_config_0.zip.tmp";
        fd_ = fopen(fn.c_str(), "w");
        if (!fd_) {
            int e = errno;
            LOG_ERROR("create name_config_0.zip.tmp failed because of %s", strerror(e));
            return false;
        }
    }

    if (bufSize > 0) {
        while (bufSize > 0) {
            size_t w = fwrite((void *) buf, 1, bufSize, fd_);
            bufSize -= w;
            buf += w;
        }
    } else {
        if (fclose(fd_) != 0) {
            int e = errno;
            LOG_INFO("fclose named_config_0.zip.tmp failed because of %s", strerror(e));
            return false;
        }
        fd_ = NULL;
    }

    return true;
}

bool FileBackup::saveFinished() {
    if (fd_ == NULL) {
        return true;
    }
    if (fclose(fd_) != 0) {
        int e = errno;
        LOG_ERROR("saveFinished close failed because of %s", strerror(e));
        return false;
    }
    fd_ = NULL;
    return true;
}

bool FileBackup::zipEtcDir() {
    bool ret = true;
    string cmd = "zip -r ";
    cmd = cmd + etcBackupPath + "/named_etc_0.zip " + etcPath;
    FILE *fs = popen(cmd.c_str(), "r");
    if (fs == NULL) {
        LOG_ERROR("popen failed");
        return false;
    }

    std::string warnMessage("zip warning");
    char buf[1024];
    std::string lastMessage;
    while (fgets(buf, 1024 - 1, fs) != NULL) {
        lastMessage = buf;
        if (lastMessage.find(warnMessage) != std::string::npos) {
            LOG_WARN("zip error : %s", buf);
            ret = false;
        }
    }
    return ret;
}

bool FileBackup::backupRecvFile() {
    LOG_DEBUG("backup recv file to %s", recvConfigPath.c_str());
    char filename1[255];
    char filename2[255];
    int i;

    for (i = maxFileBackupNumber; i > 0; i--) {
        snprintf(filename1, 255, "%s.%d", this->fileName.c_str(), i);
        snprintf(filename2, 255, "%s.%d", this->fileName.c_str(), i - 1);
        string fn1 = recvConfigPath + "/" + filename1;
        string fn2 = recvConfigPath + "/" + filename2;

        if (rename(fn2.c_str(), fn2.c_str()) != 0) {
            int e = errno;
            if (e != ENOENT) {
                LOG_INFO("backup file failed because of %s", strerror(e));
            }
        }
    }

    string src = recvConfigPath + "/" + this->fileName + ".tmp";
    string dst = recvConfigPath + "/" + this->fileName + ".0";
    rename(src.c_str(), dst.c_str());
    return true;
}

bool FileBackup::backupOldFile() {
    char filename1[255];
    char filename2[255];
    int i;

    for (i = maxFileBackupNumber; i > 0; i--) {
        snprintf(filename1, 255, "%s.%d", this->fileName.c_str(), i);
        snprintf(filename2, 255, "%s.%d", this->fileName.c_str(), i - 1);
        string fn1 = etcBackupPath + "/" + filename1;
        string fn2 = etcBackupPath + "/" + filename2;
        if (rename(fn2.c_str(), fn1.c_str()) != 0) {
            int e = errno;
            if (e != ENOENT) {
                LOG_ERROR("backup ect failed because of %s", strerror(e));
            }
        }
    }
    return true;
}

bool FileBackup::checkFileMd5(string srcMd5) {
    string recvFile = recvConfigPath + "/" + this->fileName + ".tmp";
    string fileMd5;
    {
        ifstream in(recvFile.c_str());
        MD5 md5;
        md5.reset();
        md5.update(in);
        fileMd5 = md5.toString();
    }

    if (fileMd5.size() == 0) {
        LOG_ERROR("compute MD5 fail");
        return false;
    }

    transform(fileMd5.begin(), fileMd5.end(), fileMd5.begin(), ::toupper);
    transform(srcMd5.begin(), srcMd5.end(), srcMd5.begin(), ::toupper);

    if (fileMd5 != srcMd5) {
        LOG_ERROR("MD5 Digest checksum error, compute md5=%s, recv md5 = %s",
                  fileMd5.c_str(), srcMd5.c_str());
        return false;
    }

    return true;
}

bool FileBackup::saveFileToPath() {
    FILE *srcFd;
    FILE *dstFd;

    string srcFile = recvConfigPath + "/" + this->fileName + ".0";
    string dstFile = this->savePath;

    srcFd = fopen(srcFile.c_str(), "r");
    if (!srcFd) {
        int e = errno;
        LOG_ERROR("open file %s failed because of %s", srcFile.c_str(), strerror(e));
        return false;
    }

    dstFd = fopen(dstFile.c_str(), "w");
    if (!dstFd) {
        int e = errno;
        LOG_ERROR("crate file %s failed because of %s", dstFile.c_str(), strerror(e));
        return false;
    }

    char buff[1024] = {0};
    size_t rsize;
    while ((rsize = fread(buff, 1, 1024, srcFd)) > 0) {
        size_t wsize = fwrite((void *) buff, 1, rsize, dstFd);
        if (rsize != wsize) {
            int e = errno;
            LOG_ERROR("copy file %s to %s failed because of %s", srcFile.c_str(),
                      dstFile.c_str(), strerror(e));
            return false;
        }
    }
    fclose(dstFd);
    fclose(srcFd);
    return true;
}

bool FileBackup::unzipFile() {
    bool ret = true;
    string decompdir = recvConfigPath + "/named_config_0";
    LOG_DEBUG("delete dir = %s", decompdir.c_str());
    delDirIfExist(decompdir.c_str());
    string cmd = "unzip -d";
    cmd = cmd + decompdir + " " + recvConfigPath + "/name_config_0.zip";
    FILE *fs = popen(cmd.c_str(), "r");
    if (fs == NULL) {
        LOG_ERROR("popen Failed");
        return false;
    }

    char buf[1024];
    while (fgets(buf, 1024 - 1, fs) != NULL) {
        if (memcmp(buf, "error", 5) == 0) {
            LOG_ERROR("unzip : %s", buf);
            ret = false;
        }
    }
    return ret;
}

bool FileBackup::mvNewConf2Etc() {
    bool ret = false;
    string cmd = "cp -rf ";
    string src = recvConfigPath + "/named_config_0/*";
    string dst = etcPath;
    cmd = cmd + src + " " + dst;
    int status = -1;
    if ((status = system(cmd.c_str())) == -1) {
        LOG_ERROR("execute command failed: %s", cmd.c_str());
        return false;
    } else {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status) == 0) {
                ret = true;
            } else {
                LOG_ERROR("execute command failed:%s", cmd.c_str());
            }
        } else {
            LOG_ERROR("execute command failed:%s", cmd.c_str());
            ret = false;
        }
    }
    string rmCmd = "rm -rf";
    rmCmd += recvConfigPath + "/named_config_0";
    system(rmCmd.c_str());
    return ret;
}
