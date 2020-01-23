//
// Created by System Administrator on 2020/1/20.
//

#include "Protocol.h"

Protocol::Protocol(std::string& conn_key) : aes256gcm_crypto(conn_key) {}

uint32_t Protocol::EncryptHeader(ProtocolHeader* header)
{
    uint32_t original_len = header->PADDING_LENGTH + header->PAYLOAD_LENGTH;

    unsigned char encrypted_length[8];
    uint64_t tag_len = 0;

    aes256gcm_crypto.encryptData(header->NONCE, (unsigned char*)&header->PAYLOAD_LENGTH,
                                 sizeof(header->PAYLOAD_LENGTH) + sizeof(header->PADDING_LENGTH), encrypted_length, &tag_len, header->LEN_TAG);

    memcpy(&header->PAYLOAD_LENGTH, encrypted_length, 8);

    return original_len;
}


void Protocol::EncryptPayload(ProtocolHeader *header)
{

    uint64_t tag_len = 0;

    // set NONCE random value
    randombytes_buf(header->NONCE, sizeof(header->NONCE));

    // encrypt data using this NONCE and preset key
    aes256gcm_crypto.encryptData(header->NONCE, header->Payload(),
                                 header->PAYLOAD_LENGTH, encryptedData, &tag_len, header->PAYLOAD_TAG);

    // copy back cause it can't encrypt inplace
    memcpy(header->Payload(), encryptedData, header->PAYLOAD_LENGTH);

    addObfuscation(header);
}

bool Protocol::DecryptPayload(ProtocolHeader *header)
{
    if (header->PAYLOAD_LENGTH > MAX_BUFFSIZE) return false;

    bool res = aes256gcm_crypto.decryptData(header->NONCE, header->Payload(),
                                            header->PAYLOAD_LENGTH, decryptedData,
                                            header->PAYLOAD_TAG);


    if (res)
    {
        memcpy(header->Payload(), decryptedData, header->PAYLOAD_LENGTH);
        return true;
    }

    return false;
}

uint32_t Protocol::DecryptHeader(ProtocolHeader *header)
{
    struct {
        uint32_t PAYLOAD_LENGTH;
        uint32_t PADDING_LENGTH;
    } len;


    bool res = aes256gcm_crypto.decryptData(header->NONCE,
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

// (optional) add padding at the tail of payload
// we don't need to encrypt this part
void Protocol::addObfuscation(ProtocolHeader *header)
{
    if (header->PAYLOAD_LENGTH > OBF_THRESHOLD)
    {
        header->PADDING_LENGTH = 0;
        return;
    }

    auto paddle_len = RandomNumberGenerator::GetRandomIntegerBetween(OBF_MINPADDLE, OBF_MAXPADDLE);

    randombytes_buf(header->Payload() + header->PAYLOAD_LENGTH, paddle_len);

    header->PADDING_LENGTH = (uint32_t)paddle_len;
}