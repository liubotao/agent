//
// Created by liubotao on 15/10/19.
//

#ifndef AGENT_BASE64_H
#define AGENT_BASE64_H


#include <string>
#include <string.h>

using  namespace std;

class Base64 {

public:
    static string EncodeBase64(const string strSource);
    static string DecodeBase64(const string strSource);

};


#endif //AGENT_BASE64_H
