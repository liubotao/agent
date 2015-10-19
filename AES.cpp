//
// Created by liubotao on 15/10/19.
//

#include "AES.h"
#include <openssl/aes.h>
#include <openssl/err.h>
#include <iostream>


string AESDecode(const char *crypttext, int length, const char *key) {
    if (strcmp(key, "") == 0) {
        return "";
    }
    int out_len = length;
    unsigned char iv[AES_BLOCK_SIZE + 1] = "6543210987654321";

    char *out = (char *) malloc(sizeof(char) * out_len + 1);
    memset(out, 0, out_len + 1);
    AES_KEY aes;
    if (AES_set_decrypt_key((unsigned char*) key, 128, &aes) < 0) {
        return "";
    }

    AES_cbc_encrypt((unsigned char*) crypttext, (unsigned char*) out, out_len,
                    &aes, (unsigned char*) iv, AES_DECRYPT);

    string result = out;
    free(out);
    return result;
}
