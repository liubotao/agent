//
// Created by liubotao on 15/9/30.
//

#include "Util.h"


string trim(string &s) {
    if (s.length() == 0) return s;
    size_t begin = s.find_first_not_of(" \a\b\f\n\r\t\v");
    size_t end = s.find_last_not_of(" \a\b\f\n\r\t\v");
    if (begin == string::npos) return "";
    return string(s, begin, end - begin + 1);
}

string getFileName(string &fullPath) {
    int index = fullPath.find_last_of("/");
    if (index == string::npos) {
        return "";
    }
    return fullPath.substr(index + 1);
}

string getFilePath(string &fullPath) {
    int index = fullPath.find_last_of("/");
    if (index == string::npos) {
        return "";
    }

    return fullPath.substr(0, index);
}