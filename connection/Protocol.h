#pragma once

#include "ProtocolHeader.h"
#include "AES-256-GCM.h"
#include <cstring>
#include <string>
#include "../utils/RandomNumberGenerator.h"

#define MAX_BUFFSIZE 1500

// add padding to the packet when the original size of the packet is lower than OBF_THRESHOLD
#define OBF_THRESHOLD 250
/*
 * the length of the padding bytes is chosen randomly between  OBF_MINPADDLE and OBF_MAXPADDLE
 */
#define OBF_MINPADDLE 250
#define OBF_MAXPADDLE 700


struct Protocol {
public:

    Protocol(std::string& conn_key);

    uint32_t EncryptHeader(ProtocolHeader* header);

    uint32_t DecryptHeader(ProtocolHeader *header);

    void EncryptPayload(ProtocolHeader *header);

    bool DecryptPayload(ProtocolHeader *header);



private:
    AES256GCM aes256gcm_crypto;
    unsigned char encryptedData[MAX_BUFFSIZE] = {0};
    unsigned char decryptedData[MAX_BUFFSIZE] = {0};

    void addObfuscation(ProtocolHeader *header);

};


