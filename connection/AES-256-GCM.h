#pragma once

#include <sodium.h>

#define IN
#define OUT
class AES256GCM {

public:

    /*
     *  -- crypto_aead_chacha20poly1305_KEYBYTES 32U
     *  -- crypto_aead_chacha20poly1305_NPUBBYTES 12U
     *  -- crypto_aead_chacha20poly1305_ABYTES 16U
     */
    static void encryptData(unsigned char key[crypto_aead_aes256gcm_KEYBYTES], unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES],
                            IN unsigned char *original_data, IN uint64_t original_data_length, OUT unsigned char *encrypted_data, OUT uint64_t *tag_length, OUT unsigned char *tag_out)
    {


        crypto_aead_aes256gcm_encrypt_detached(encrypted_data, tag_out, (unsigned long long*)tag_length,
                                               original_data, original_data_length,
                                               NULL, 0,
                                               NULL, nonce, key);

    }


    /*
     *  -- crypto_aead_aes256gcm_KEYBYTES 32U
     *  -- crypto_aead_aes256gcm_NPUBBYTES 12U
     *  -- crypto_aead_aes256gcm_NPUBBYTES 16U
     */
    static bool decryptData(unsigned char key[crypto_aead_aes256gcm_KEYBYTES], unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES],
                            IN unsigned char *encrypted_data, IN uint64_t encrypted_data_length, OUT unsigned char *decrypted_data, IN unsigned char *tag_in)
    {


        int res =  crypto_aead_aes256gcm_decrypt_detached(decrypted_data, NULL, encrypted_data,
                                                          encrypted_data_length, tag_in,
                                                          NULL, 0, nonce, key);

        if (res != 0)
        {
            return false;
        }

        return true;

    }

};


