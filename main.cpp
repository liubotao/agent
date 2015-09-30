//
// Created by liubotao on 15/9/30.
//

#include "Util.h"

int main() {
    string path = "Hello.txt";
    string filename = getFileName(path);
    cout << filename << endl;
    string filepath = getFilePath(path);
    cout << filepath << endl;
    string word = "hello  good bye ";
    cout << word.length() << endl;
    cout << trim(word) << endl;
    cout << trim(word).length() << endl;
    return 0;
}
