#pragma once

#include <cstdint>

// 52 bytes
#pragma pack(8)
struct ProtocolHeader {

    unsigned char NONCE[12];
    unsigned char LEN_TAG[16];
    unsigned char PAYLOAD_TAG[16];
    uint32_t PAYLOAD_LENGTH;
    uint32_t PADDING_LENGTH;

    static uint8_t Size() {
        return sizeof(ProtocolHeader);
    }

    unsigned char* Payload()
    {
        return NONCE + Size();
    }

};
#pragma pack()