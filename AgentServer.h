//
// Created by liubotao on 15/10/19.
//

#ifndef AGENT_AGENTSERVER_H
#define AGENT_AGENTSERVER_H

#include  <string.h>
#include  <string>
#include <vector>

using namespace std;

class FileBackup;

void init_daemon();
void split(const string& src, const string& separator, vector<string>& desc);
bool split_cmd(string src, char* cmdcode, unsigned char* md5, long* filesize, char* cmd);

int  exec_cmd(string& command);
bool load_config();
string rand_str(const int len);
int recvFileTimeout(int fd, ssize_t filesize, long msec, FileBackup& fb);
int recvTimeout(int fd, char* buf, ssize_t size, long msec);
#endif //AGENT_AGENTSERVER_H
