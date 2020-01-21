#pragma once

#include "ProtocolHeader.h"
#include "AES-256-GCM.h"
#include <cstring>

#define MAX_BUFFSIZE 1500

struct Protocol {
public:

    uint32_t EncryptHeader(ProtocolHeader* header)
    {
        uint32_t original_len = header->PADDING_LENGTH + header->PAYLOAD_LENGTH;

        unsigned char encrypted_length[8];
        uint64_t tag_len = 0;

        AES256GCM::encryptData(key, header->NONCE, (unsigned char*)&header->PAYLOAD_LENGTH,
                                             sizeof(header->PAYLOAD_LENGTH) + sizeof(header->PADDING_LENGTH), encrypted_length, &tag_len, header->LEN_TAG);

        memcpy(&header->PAYLOAD_LENGTH, encrypted_length, 8);

        return original_len;
    }


    void EncryptPayload(ProtocolHeader *header)
    {

        uint64_t tag_len = 0;

        // set NONCE random value
        randombytes_buf(header->NONCE, sizeof(header->NONCE));

        // encrypt data using this NONCE and preset key
        AES256GCM::encryptData(key, header->NONCE, header->Payload(),
                                             header->PAYLOAD_LENGTH, encryptedData, &tag_len, header->PAYLOAD_TAG);

        // copy back cause it can't encrypt inplace
        memcpy(header->Payload(), encryptedData, header->PAYLOAD_LENGTH);

    }

    inline bool DecryptPayload(ProtocolHeader *header)
    {
        if (header->PAYLOAD_LENGTH > MAX_BUFFSIZE) return false;

        bool res = AES256GCM::decryptData(key, header->NONCE, header->Payload(),
                                                        header->PAYLOAD_LENGTH, decryptedData,
                                                        header->PAYLOAD_TAG);


        if (res)
        {
            memcpy(header->Payload(), decryptedData, header->PAYLOAD_LENGTH);
            return true;
        }

        return false;
    }

    inline uint64_t DecryptHeader(ProtocolHeader *header)
    {
        struct {
            uint32_t PAYLOAD_LENGTH;
            uint32_t PADDING_LENGTH;
        } len;


        bool res = AES256GCM::decryptData(key, header->NONCE,
                                                        (unsigned char *) &header->PAYLOAD_LENGTH,
                                                        sizeof(header->PAYLOAD_LENGTH) +
                                                        sizeof(header->PADDING_LENGTH), (unsigned char *) &len,
                                                        header->LEN_TAG);

        if (res) {
            memcpy(&header->PAYLOAD_LENGTH, &len, sizeof(len));
            return len.PAYLOAD_LENGTH + len.PADDING_LENGTH;
        }

        return 0;
    }


private:
    unsigned char key[crypto_aead_aes256gcm_KEYBYTES] = {0};
    unsigned char encryptedData[MAX_BUFFSIZE];
    unsigned char decryptedData[MAX_BUFFSIZE];
};


