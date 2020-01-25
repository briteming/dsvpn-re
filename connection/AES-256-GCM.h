#pragma once

#include <sodium.h>
#include <string>
#include <cstring>
#define IN
#define OUT
class AES256GCM {

public:

    AES256GCM(std::string conn_key) {
        //set the last digit to 0
        auto max_copy_len = crypto_aead_aes256gcm_KEYBYTES - 2;
        memcpy(this->key, conn_key.c_str(), conn_key.length() > max_copy_len ? max_copy_len : conn_key.length());
    }

    /*
     *  -- crypto_aead_aes256gcm_KEYBYTES 32U
     *  -- crypto_aead_aes256gcm__NPUBBYTES 12U
     *  -- crypto_aead_aes256gcm__ABYTES 16U
     */
    void encryptData(unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES],
                            IN unsigned char *original_data, IN uint64_t original_data_length, OUT unsigned char *encrypted_data, OUT uint64_t *tag_length, OUT unsigned char *tag_out)
    {


        crypto_aead_aes256gcm_encrypt_detached(encrypted_data, tag_out, (unsigned long long*)tag_length,
                                               original_data, original_data_length,
                                               nullptr, 0,
                                               nullptr, nonce, key);

    }


    /*
     *  -- crypto_aead_aes256gcm_KEYBYTES 32U
     *  -- crypto_aead_aes256gcm_NPUBBYTES 12U
     *  -- crypto_aead_aes256gcm_NPUBBYTES 16U
     */
    bool decryptData(unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES],
                            IN unsigned char *encrypted_data, IN uint64_t encrypted_data_length, OUT unsigned char *decrypted_data, IN unsigned char *tag_in)
    {


        int res =  crypto_aead_aes256gcm_decrypt_detached(decrypted_data, nullptr, encrypted_data,
                                                          encrypted_data_length, tag_in,
                                                          nullptr, 0, nonce, key);

        if (res != 0)
        {
            return false;
        }

        return true;

    }

private:
    unsigned char key[crypto_aead_aes256gcm_KEYBYTES] = {0};

};


