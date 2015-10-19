//
// Created by liubotao on 15/10/19.
//

#include <dirent.h>
#include <unistd.h>
#include <sys/time.h>
#include <openssl/bio.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <openssl/evp.h>
#include "AgentServer.h"
#include "macro_define.h"
#include "Logging.h"
#include "INIParser.h"
#include "Util.h"
#include "AES.h"
#include "RSA.h"
#include "FileBackup.h"


using namespace std;

#define  SERVER_PORT 9625
#define  LENGTH_OF_LISTEN_QUEUE 10

#define CLOSE_SOCKET(fd) \
		if(fd != 0) {\
			close(fd);\
			fd = 0;\
			LOG_DEBUG("socket closed\n");\
		}\
		continue;

#define RESPONSE(fd,buf,len,msg)  \
		if (send(fd, buf, len, 0) == -1) {\
			LOG_ERROR("send message to client error, DATA[%s]",msg); \
			CLOSE_SOCKET(fd);\
			continue; \
		}\
		LOG_DEBUG("send message to client, DATA[%s]",msg);

string pubkey = "";

vector<string> manager_ips_arr;

int max_file_backup_number = 10;
string etc_path;
string etc_backup_path;
string recv_config_path;
string dns_sbin_path;
string dns_config_file;
string auth_key;

string log_file;
string log_path;

bool mkDirR(const char* dirname)
{
    int len = strlen(dirname);
    char* path = new char[len+1];
    memcpy(path, dirname, len);
    path[len] = '\0';
    for(int i = 1;i < len;i++) {
        if(path[i] == '/') {
            path[i] = '\0';
            if(opendir(path) == NULL) {
                LOG_DEBUG("Create path : %s",path);
                if(mkdir(path, 0755) == -1) {
                    int e = errno;
                    LOG_ERROR("Create path %s failed, %s", path, strerror(e));
                    return false;
                }
            }
            path[i] = '/';
        }
    }
    if(opendir(path) == NULL) {
        LOG_DEBUG("create path : %s",path);
        if(mkdir(path, 0755) == -1) {
            int e = errno;
            LOG_ERROR("Create path %s failed, %s", path, strerror(e));
            return false;
        }
    }
    return true;
}

bool createDirIfNotExist(const char* dirname) {
    if(dirname == NULL)
        return true;
    DIR *d = NULL;
    if( (d = opendir(dirname)) == NULL) {
        if (errno == ENOENT) {
            if(mkDirR(dirname)) {
                return true;
            }else {
                LOG_ERROR("createDirIfNotExist error, filename = %s",dirname);
                return false;
            }
        }
    }
    closedir(d);
    LOG_DEBUG("createDirIfNotExist filename is exist");
    return true;
}

bool load_config() {
    INIParser ini_parser;
    if (ini_parser.ReadINI("config.ini") == 1) {
        string aes_key = "Sdjswlsjf93402ks";
        string manager_ips = ini_parser.GetValue("default", "manager_ips");
        string b = ini_parser.GetValue("default", "max_file_backup_number");
        if (b == "") {
            max_file_backup_number = 50;
        } else {
            int  n = atoi(b.c_str());
            max_file_backup_number = (n < 10 ? 10 : n);
        }

        etc_backup_path = ini_parser.GetValue("default", "etc_backup_path");
        recv_config_path = ini_parser.GetValue("default", "recv_config_path");

        if (!createDirIfNotExist(etc_backup_path.c_str())) {
            printf("load config error, can not crate etc_backup_path:%s\n", etc_backup_path.c_str());
            return false;
        }

        if (!createDirIfNotExist(recv_config_path.c_str())) {
            printf("load config error, can not crate recv_config_path:%s\n", recv_config_path.c_str());
            return false;
        }

        pubkey = ini_parser.GetValue("security", "pubkey_path");
        auth_key = ini_parser.GetValue("security", "auth_key");
        string log_file_path = ini_parser.GetValue("log", "log_file_path");

        if (trim(pubkey).length() == 0) {
            printf("load config error, pubkey_path can not be empty\n");
            return false;
        }

        if (manager_ips.length() == 0) {
            printf("load config error, manager_ips can not be empty\n");
            return false;
        }

        split(manager_ips, ",", manager_ips_arr);

        if (trim(auth_key).length() != 16) {
            printf("load config error, auth_key must be 16 characters\n");
            return false;
        }

        if (trim(log_file_path).length() == 0) {
            log_file = "lvs-agent";
            log_path = ".";
        } else {
            log_path = getFilePath(log_file_path);
            log_file = getFileName(log_file_path);
        }

        if (!createDirIfNotExist(log_path.c_str())) {
            printf("load config error, can not crate log folder:%s\n", log_path.c_str());
            return false;
        }

        ini_parser.Clear();
        printf("load config success\n");
        return true;
    }

    return false;
}

bool is_manager_ip(string ip) {
    if (manager_ips_arr.size() == 0) {
        return true;
    }

    for (vector<string>::iterator iter = manager_ips_arr.begin(); iter != manager_ips_arr.end();
            iter++) {
        if (*iter == ip) {
            return true;
        }
    }

    return false;
}

int recvFileTimeout(int fd, ssize_t filesize, long msec, FileBackup& fileBackup) {

    usleep(10*1000);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t startTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    uint64_t endTime = 0;

    char recvbuf[1024];
    while (filesize > 0) {
        gettimeofday(&tv, NULL);
        endTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        ssize_t nread = read(fd, (void*)recvbuf, 1024);
        if (nread >= 0) {
            if (!fileBackup.saveNewFile(recvbuf, nread)) {
                LOG_ERROR("save_new_file error");
                return -1;
            }

            filesize -= nread;
            if (nread == 0 && filesize != 0) {
                LOG_ERROR("recv filesize error");
                return -1;
            }

            startTime = endTime;
        } else {
            int e = errno;
            if (e != EAGAIN) {
                LOG_ERROR("recv file error because of %s", strerror(e));
                return -1;
            } else {
                usleep(msec * 100);
            }
        }

        if (endTime - startTime > (uint64_t) msec) {
            LOG_WARN("recv file timeout");
            break;
        }
    }

    fileBackup.saveFinished();
    if (filesize == 0) {
        return 0;
    }

    return 1;
}

int recvTimeout(int fd, char* buf, ssize_t size, long msec) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t startTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    uint64_t endTime = 0;

    char* recvbuf = buf;

    while(size > 0) {
        gettimeofday(&tv, NULL);
        endTime = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        ssize_t nread = read(fd, (void*) recvbuf, size);
        if (nread >= 0) {
            size -= nread;
            recvbuf += nread;
            if (nread == 0 && size != 0) {
                LOG_ERROR("recv filesize error");
                return -1;
            }

            startTime = endTime;
        } else {
            int e = errno;
            if (e != EAGAIN) {
                LOG_ERROR("recv file error because of %s", strerror(e));
                return -1;
            } else {
                usleep(msec*100);
            }
        }

        if (endTime - startTime > (uint64_t)msec) {
            LOG_WARN("recv file timeout");
            break;
        }


    }

    if (size == 0) {
        return 0;
    }

    return -1;

}

int Base64Decode(char *input, int length, char* output, bool with_new_line)
{
    BIO * b64 = NULL;
    BIO * bmem = NULL;

    b64 = BIO_new(BIO_f_base64());
    if(!with_new_line) {
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    }
    bmem = BIO_new_mem_buf(input, length);
    bmem = BIO_push(b64, bmem);
    BIO_read(bmem, output, length);
    BIO_free_all(bmem);
    BIO_free(b64);

    return 0;
}


void init_daemon(void) {
    pid_t pid;

    pid = fork();
    if (pid > 0) {
        char pid_str[20];
        sprintf(pid_str, "%d", pid);
        FILE *fp;
        if ((fp = fopen("/var/run/dnssync.pid", "w")) == NULL) {
            printf("文件还未创建!\n");
        }

        fwrite(pid_str, strlen(pid_str), 1, fp);
        fwrite("\n", 1, 1, fp);
        fclose(fp);
        exit(0);
    } else if (pid < 0) {
        perror("创建子进程失败\n");
        exit(1);
    } else if (pid == 0) {
        setsid();
        return;
    }
}

bool  get_cmdcode(string &src, string &cmdcode) {
    string::size_type start = 0;
    size_t index = src.find_first_of(":");
    if (index == string::npos) {
        LOG_ERROR("get cmdcode failed, str = ", src.c_str());
        return false;
    }

    cmdcode = src.substr(start, index - start);
    LOG_DEBUG("cmdcode:[%s]", cmdcode.c_str());
    return true;
}

bool get_parameter(string src, string &parameter) {
    string str = src;
    size_t index = src.find_first_of(":");
    if (index == string::npos) {
        LOG_ERROR("get cmdcode failed, str = ", str.c_str());
        return false;
    }

    parameter = src.substr(index + 1);
    LOG_DEBUG("parameter:[%s]", parameter.c_str());

    return true;
}

int exec_cmd(string& command) {
    int status = system(command.c_str());
    LOG_INFO("exec command %s result: %d", command.c_str(), status);
    if (status == -1) {
        LOG_ERROR("exec command failed!");
        return -1;
    } else {
        if (WIFEXITED(status) == 0) {
            if(WEXITSTATUS(status) == 0) {
                LOG_INFO("exec command successful");
                return 0;
            } else {
                LOG_ERROR("exec command failed 2 !");
                return -2;
            }
        } else {
            LOG_ERROR("exec command failed 3!");
            return -3;
        }
    }

}

int system_exec_cmd(string& command) {
    int status = system(command.c_str());
    LOG_INFO("exec command %s result:%d",command.c_str(), status);
    if( status == -1) {
        LOG_ERROR("exec command failed !");
        return -1;
    } else {
        if(WIFEXITED(status)) {
            if(WEXITSTATUS(status) == 0) {
                LOG_INFO("exec command successful");
                return 0;
            } else {
                LOG_ERROR("exec command failed 2 !");
                return -2;
            }
        } else {
            LOG_ERROR("exec command failed 3!");
            return -3;
        }
    }
}

bool system_exec_cmdx(string& command,string &result) {

    FILE *fs = popen(command.c_str(), "r");
    if(fs == NULL) {
        LOG_ERROR("popen Failed");
        return false;
    }

    char buf[1024]={0};
    while(fgets(buf,1024-1,fs) != NULL) {
        result+=buf;
    }

    LOG_INFO("exec command: %s, output : %s",command.c_str(), result.c_str());
    return true;
}

bool get_sendfile_param(string src,string &md5,string &savepath,ssize_t &filesize){
    size_t index = src.find_first_of("|");
    size_t start = 0;
    if (index == string::npos) {
        LOG_ERROR("get file md5 fail, str = ",src.c_str());
        return false;
    }
    md5 = src.substr(start, index - start);
    start = index + 1;
    index = src.find_first_of("|", start);
    if (index == string::npos) {
        LOG_ERROR("get save path fail, str = ",src.c_str());
        return false;
    }
    savepath=src.substr(start, index - start);
    start = index + 1;
    filesize = atol(src.substr(start).c_str());
    if(filesize <= 0) {
        LOG_ERROR("get file size fail, str = ",src.c_str());
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    int server_fd, client_fd;
    int yes = 1;
    int timeout = 1;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    if (!load_config()) {
        LOG_ERROR("agent server start failure: load config error");
        exit(1);
    }

    LOG_INIT(LL_DEBUG, log_file.c_str(), log_path.c_str());
    LOG_INFO("agent server start...");
    LOG_INFO("load config success");

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LOG_ERROR("agent server start failure: crate socket error !\n");
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = htons(INADDR_ANY);

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(int));
    //setsockopt(server_fd, SOL_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(timeout));

    if (::bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR("agent server start failure: bind to port %d failure!", SERVER_PORT);
        exit(1);
    }

    if (listen(server_fd, LENGTH_OF_LISTEN_QUEUE) < 0) {
        LOG_ERROR("agent server start failure: listen failure");
        exit(1);
    }

    printf("agent server start success, listen port : %d , listen queue : %d", SERVER_PORT,LENGTH_OF_LISTEN_QUEUE);
    LOG_INFO("agent server start success, listen port : %d , listen queue : %d", SERVER_PORT,LENGTH_OF_LISTEN_QUEUE);


    while (1) {
        string send_msg;
        socklen_t length = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *) & client_addr, &length);
        if (client_fd < 0) {
            LOG_ERROR("accpet error!");
            continue;
        }

        int flags = fcntl(client_fd, F_GETFL, 0);
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

        LOG_INFO("recived a connection from client, ip : %s, port : %d", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        string client_ip = inet_ntoa(client_addr.sin_addr);
        if (!is_manager_ip(client_ip)) {
            LOG_WARN("recevieved a illegal connection, will close it");
            RESPONSE(client_fd, "reject", strlen("reject"), "reject");
            CLOSE_SOCKET(client_fd);
        }

        setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, (char*) &yes, sizeof(int));

        char code[16] = {16};
        if (recvTimeout(client_fd, code, 16, 1000) != 0) {
            RESPONSE(client_fd, "recv error", strlen("recv error"), "recv error");
            LOG_ERROR("received getcode error");
            CLOSE_SOCKET(client_fd);
        }

        LOG_DEBUG("received DATA:[%s]", code);

        if (memcmp(code, auth_key.c_str(), auth_key.length()) != 0) {
            send_msg = "auth key error";
            RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
            LOG_WARN("recvived non-recognition auth key");
            CLOSE_SOCKET(client_fd);
        }

        string check_code = rand_str(16);
        string en_check_code = EncodeRSA(pubkey, check_code);
        RESPONSE(client_fd, en_check_code.c_str(), en_check_code.length(), check_code.c_str());

        char data[1024] = {0};
        ssize_t nread = 0;
        usleep(100 * 1000);
        for (int i = 0; i < 20; i++) {
            nread = read(client_fd, (void*) data, 1023);
            if (nread > 0) {
                break;
            }

            usleep(50 * 1000);
        }

        LOG_DEBUG("received DATA length = %d", nread);
        if (nread <= 0) {
            RESPONSE(client_fd, "recv error", strlen("recv error"), "after send check code, recv command failure");
            LOG_WARN("received sendfile command error");
            CLOSE_SOCKET(client_fd);
        }

        LOG_DEBUG("recevied DATA [%s]", data);

        char afterbase64[1024] = {0};
        Base64Decode(data, nread, afterbase64, false);

        string receive_str = AESDecode(afterbase64, nread, check_code.c_str());
        if (receive_str.size() == 0) {
            RESPONSE(client_fd, "decode message error", strlen("decode message error"),"decode message error");
            LOG_ERROR("recvived command decode failed , str = ", receive_str.c_str());
            CLOSE_SOCKET(client_fd);
            continue;
        }

        LOG_DEBUG("after decode DATA:[%s]", receive_str.c_str());

        string cmdcode;
        if (!get_cmdcode(receive_str, cmdcode)) {
            LOG_ERROR("get cmdcode failed, receive_str = ", receive_str.c_str());
            string error_msg = "command error";
            RESPONSE(client_fd, error_msg.c_str(), error_msg.length(), error_msg.c_str());
            CLOSE_SOCKET(client_fd);
            continue;
        }

        string parameter;
        if (!get_parameter(receive_str, parameter)) {
            send_msg = "command parameter error";
            LOG_ERROR("get parameter failed failed, receive_str = ", receive_str.c_str());
            RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
            CLOSE_SOCKET(client_fd);
            continue;
        }

        if (cmdcode == "sendfile") {
            if (parameter.length() == 0) {
                send_msg = "command parameter error";
                LOG_ERROR("parmater error, paramter is empty, str = ", receive_str.c_str());
                RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                CLOSE_SOCKET(client_fd);
            }

            string md5, savepath;
            ssize_t filesize;
            if (!get_sendfile_param(parameter, md5, savepath, filesize)) {
                send_msg="parameter error";
                LOG_ERROR("parameter error,get parameter failed,str = ", receive_str.c_str());
                RESPONSE(client_fd,send_msg.c_str(), send_msg.length(), send_msg.c_str());
                CLOSE_SOCKET(client_fd);
            }

            if (trim(savepath) == "") {
                send_msg = "savepath parameter error";
                LOG_ERROR("parameter error , savepath is emtpy , str = ", receive_str.c_str());
                RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                CLOSE_SOCKET(client_fd);
            }

            RESPONSE(client_fd, "recv:ready", strlen("recv:ready"), "recv:ready");
            {
                FileBackup fb;
                fb.setSavePath(savepath);
                int ret = recvFileTimeout(client_fd, filesize, 1000, fb);
                if (ret != 0) {
                    string err = (ret < 0 ? "error" : "timeout");
                    LOG_ERROR("recv file %s", err.c_str());
                    string e = "recv file";
                    e += err;
                    RESPONSE(client_fd, e.c_str(), e.length(), e.c_str());
                }

                if(!fb.checkFileMd5(md5)) {
                    send_msg="recvfile:checksum file error";
                    LOG_ERROR("check file error");
                    RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                    CLOSE_SOCKET(client_fd);
                }

                if(!fb.backupRecvFile()){
                    send_msg="recvfile:backup file error";
                    LOG_ERROR("backup file error");
                    RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                    CLOSE_SOCKET(client_fd);
                }

                fb.backupOldFile();
                if(!fb.saveFileToPath()){
                    send_msg="recvfile:backup old file error";
                    LOG_ERROR("backup old file failed");
                    RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                    CLOSE_SOCKET(client_fd);
                }
                send_msg="recvfile:success";
                RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                LOG_INFO("recv file success,%s",savepath.c_str());
            }
        } else if (cmdcode == "runcmd") {
            if (trim(parameter).length() == 0) {
                send_msg = "parameter error";
                LOG_ERROR("parameter error, str=", receive_str.c_str());
                RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                CLOSE_SOCKET(client_fd);
            }

            int ret = system_exec_cmd(parameter);
            if (ret != 0) {
                send_msg = "runcmd:fail";
                LOG_ERROR("exec command failed, COMMAND:[%s]", parameter.c_str());
                RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                CLOSE_SOCKET(client_fd);
            } else {
                send_msg="runcmd:success";
                RESPONSE(client_fd, send_msg.c_str(), send_msg.length(), send_msg.c_str());
                LOG_INFO("exec commnad success,COMMAND:[%s]",parameter.c_str());
                CLOSE_SOCKET(client_fd);
            }
        } else if(cmdcode=="runcmdx") {
            if(trim(parameter).length()==0){
                send_msg="command parameter error";
                LOG_ERROR("received command split failed, str : ", receive_str.c_str());
                RESPONSE(client_fd,send_msg.c_str(), send_msg.length(), send_msg.c_str());
                CLOSE_SOCKET(client_fd);
            }
            string result;
            if(!system_exec_cmdx(parameter,result)){
                send_msg="exec commnad fail";
                LOG_ERROR("runcmd:fail,COMMAND:%s", parameter.c_str());
                RESPONSE(client_fd,send_msg.c_str(), send_msg.length(), send_msg.c_str());
                CLOSE_SOCKET(client_fd);
            }
            else{
                result="runcmd:"+result;
                RESPONSE(client_fd, result.c_str(), result.length(), result.c_str());
                LOG_INFO("exec commnad success,COMMAND:%s",parameter.c_str());
                CLOSE_SOCKET(client_fd);
            }
        } else{
            send_msg="error command";
            RESPONSE(client_fd,send_msg.c_str(), send_msg.length(),send_msg.length());
            LOG_ERROR("error command,COMMAND:%s ",receive_str.c_str());
            CLOSE_SOCKET(client_fd);
        }

        CLOSE_SOCKET(client_fd);
    }

    close(server_fd);
    return 0;
}