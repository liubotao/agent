//
// Created by liubotao on 15/10/19.
//

#include "Base64.h"

const char* m_Base64_Table= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

string Base64::EncodeBase64(const string strSource) {

    string strEncode;
    char cTemp[4];

//行长,MIME规定Base64编码行长为76字节
    int LineLength = 0;

    for (unsigned int i = 0; i < strSource.size(); i += 3) {
        memset(cTemp, 0, 4);

        cTemp[0] = strSource[i];
        cTemp[1] = strSource[i + 1];
        cTemp[2] = strSource[i + 2];

        int len = strlen(cTemp);
        if (len == 3) {
            strEncode += m_Base64_Table[((int) cTemp[0] & 0xFC) >> 2];
            strEncode += m_Base64_Table[((int) cTemp[0] & 0x3) << 4
                                        | ((int) cTemp[1] & 0xF0) >> 4];
            strEncode += m_Base64_Table[((int) cTemp[1] & 0xF) << 2
                                        | ((int) cTemp[2] & 0xC0) >> 6];
            strEncode += m_Base64_Table[(int) cTemp[2] & 0x3F];
            if (LineLength += 4 >= 76)
                strEncode += "\r\n";
        } else if (len == 2) {
            strEncode += m_Base64_Table[((int) cTemp[0] & 0xFC) >> 2];
            strEncode += m_Base64_Table[((int) cTemp[0] & 0x3) << 4
                                        | ((int) cTemp[1] & 0xF0) >> 4];
            strEncode += m_Base64_Table[((int) cTemp[1] & 0x0F) << 2];
            if (LineLength += 4 >= 76)
                strEncode += "\r\n";
            strEncode += "=";
        } else if (len == 1) {
            strEncode += m_Base64_Table[((int) cTemp[0] & 0xFC) >> 2];
            strEncode += m_Base64_Table[((int) cTemp[0] & 0x3) << 4];
            if (LineLength += 4 >= 76)
                strEncode += "\r\n";
            strEncode += "==";
        }
        memset(cTemp, 0, 4);
    }
    return strEncode;
}

string Base64::DecodeBase64(const string strSource) {
//返回值
    string strDecode;
    char cTemp[5];

    for (unsigned int i = 0; i < strSource.size(); i += 4) {
        memset(cTemp, 0, 5);

        cTemp[0] = strSource[i];
        cTemp[1] = strSource[i + 1];
        cTemp[2] = strSource[i + 2];
        cTemp[3] = strSource[i + 3];

        int asc[4];
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < (int) strlen(m_Base64_Table); k++) {
                if (cTemp[j] == m_Base64_Table[k])
                    asc[j] = k;
            }
        }
        if ('=' == cTemp[2] && '=' == cTemp[3]) {
            strDecode += (char) (int) (asc[0] << 2 | asc[1] << 2 >> 6);
        } else if ('=' == cTemp[3]) {
            strDecode += (char) (int) (asc[0] << 2 | asc[1] << 2 >> 6);
            strDecode += (char) (int) (asc[1] << 4 | asc[2] << 2 >> 4);
        } else {
            strDecode += (char) (int) (asc[0] << 2 | asc[1] << 2 >> 6);
            strDecode += (char) (int) (asc[1] << 4 | asc[2] << 2 >> 4);
            strDecode += (char) (int) (asc[2] << 6 | asc[3] << 2 >> 2);
        }
    }
    return strDecode;
}