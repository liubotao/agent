//
// Created by liubotao on 15/10/19.
//

#ifndef AGENT_RSA_H
#define AGENT_RSA_H

#include <string>
#include <string.h>

using namespace std;

string EncodeRSA(const string& strPemFileName, const string& strData);
string DecodeRSA(const string& strPemFileName, const string& strData);


#endif //AGENT_RSA_H
