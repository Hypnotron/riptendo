#pragma once
#include <cstdint>
#include <array>
using u8 =  std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 =  std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

using u8_fast  = std::uint_fast8_t;
using u16_fast = std::uint_fast16_t;
using u32_fast = std::uint_fast32_t;
using u64_fast = std::uint_fast64_t;

using s8_fast  = std::int_fast8_t;
using s16_fast = std::int_fast16_t;
using s32_fast = std::int_fast32_t;
using s64_fast = std::int_fast64_t;

enum class Endianness {
    LITTLE,
    BIG
};

const u8 emptyArray[65536] {};

template <
        u8 byteCount, 
        typename DataType, 
        Endianness endianness = Endianness::LITTLE,
        typename AddressType>
inline DataType readBytes(AddressType address) { 
    constexpr bool little {endianness == Endianness::LITTLE};
    DataType data {0};
    for (
            s16 shiftCount = little ? 0 : (byteCount - 1) * 8;
            little ? shiftCount < (byteCount * 8) : shiftCount >= 0;
            shiftCount += (little ? 8 : -8), ++address) {
        data |= *address << shiftCount; 
    }
    return data;
}
template <typename DataType, typename AddressType>
inline DataType readBytes(
        u8 byteCount,
        Endianness endianness = Endianness::LITTLE,
        AddressType address = 0) { 
    const bool little {endianness == Endianness::LITTLE};
    DataType data {0};
    for (
            s16 shiftCount = little ? 0 : (byteCount - 1) * 8;
            little ? shiftCount < (byteCount * 8) : shiftCount >= 0;
            shiftCount += (little ? 8 : -8), ++address) {
        data |= *address << shiftCount; 
    }
    return data;
}

template <
        u8 byteCount, 
        Endianness endianness = Endianness::LITTLE, 
        typename DataType, 
        typename AddressType>
inline void writeBytes(AddressType address, DataType data) {
    constexpr bool little {endianness == Endianness::LITTLE};
    for (u8 byteIndex {0}; byteIndex < byteCount; ++byteIndex, ++address) {
        *address = little ? data : data >> ((byteCount - 1) * 8); 
        data = little ? data >> 8 : data << 8;
    }
}
template <typename DataType, typename AddressType>
inline void writeBytes(
        u8 byteCount, 
        Endianness endianness = Endianness::LITTLE,
        AddressType address = 0, 
        DataType data = 0) {
    const bool little {endianness == Endianness::LITTLE};
    for (u8 byteIndex {0}; byteIndex < byteCount; ++byteIndex, ++address) {
        *address = little ? data : data >> ((byteCount - 1) * 8); 
        data = little ? data >> 8 : data << 8;
    }
}

inline s8 toSigned(u8 value) {
    if (value <= INT8_MAX) {
        return value;
    }
    else {
        return static_cast<s8>(value - INT8_MIN) + INT8_MIN;
    }
}
inline s16 toSigned(u16 value) {
    if (value <= INT16_MAX) {
        return value;
    }
    else {
        return static_cast<s16>(value - INT16_MIN) + INT16_MIN;
    }
}
inline s32 toSigned(u32 value) {
    if (value <= INT32_MAX) {
        return value;
    }
    else {
        return static_cast<s32>(value - INT32_MIN) + INT32_MIN;
    }
}
inline s64 toSigned(u64 value) {
    if (value <= INT64_MAX) {
        return value;
    }
    else {
        return static_cast<s64>(value - INT64_MIN) + INT64_MIN;
    }
}

template <typename DataType>
inline void setBit(DataType& data, u8_fast bit, bool value) {
    data &= ~(1 << bit);
    data |= value << bit;
}

template <u8_fast byteCount, typename DataType>
inline DataType bitwiseReverse(const DataType& data) {
    static const std::array<u8, 256> reverseTable { 
            0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
            0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
            0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
            0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
            0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
            0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
            0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
            0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
            0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
            0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
            0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
            0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
            0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
            0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
            0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
            0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
            0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
            0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
            0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
            0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
            0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
            0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
            0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
            0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
            0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
            0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
            0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
            0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
            0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
            0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
            0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
            0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
    };

    DataType result {0};
    for (u8_fast i {0}; i < byteCount; ++i) {
        result |= 
                reverseTable[data >> (i * 8) & 0xFF] 
             << ((byteCount - i - 1) * 8);
    }
    return result;
}
