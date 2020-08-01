#pragma once
#include "file.hpp"

class WavFile : public File {
    private:

    public:
        WavFile() {}
        WavFile(const std::string& filename)
              : File(filename) {
        }

        std::string toWav(File& dest) const override {
            size_t offset {dest.start};
            for (
                    auto byte {buffer.begin() + start};
                    byte < buffer.begin() + end; 
                    ++byte) {
                dest.buffer[offset++] = *byte;
            }
            return "";
        }
        std::string fromWav(const File& source) override {
            size_t offset {start};
            for (
                    auto byte {source.buffer.begin() + source.start};
                    byte < source.buffer.begin() + source.end;
                    ++byte) {
                buffer[offset++] = *byte;
            }
            return "";
        }
};

namespace wav {
    struct Info {
        u16 channelCount;
        u32 sampleRate;
        u16 frameSize;
        u16 bitDepth;
        u32 dataSize;
    };

    template <typename BufferType>
    wav::Info parseHeader(
                const BufferType& buffer,
                const size_t offset, 
                std::string& message) {
        wav::Info info;
        info.channelCount       = readBytes<2, u16>(buffer + offset + 0x16);
        info.sampleRate         = readBytes<4, u32>(buffer + offset + 0x18);
        info.frameSize          = readBytes<2, u16>(buffer + offset + 0x20);
        info.bitDepth           = readBytes<2, u16>(buffer + offset + 0x22);
        info.dataSize           = readBytes<4, u32>(buffer + offset + 0x28);

        if (
                readBytes<4, u32, Endianness::BIG>(buffer + offset) 
             != 0x52494646) {
            message +=
                    "WARNING: missing RIFF magic (@"
                  + std::to_string(offset)
                  + ")!\n";
        }
        if (
                readBytes<4, u32, Endianness::BIG>(buffer + offset + 0x08)
             != 0x57415645) {
            message +=
                    "WARNING: missing WAVE magic (@"
                  + std::to_string(offset + 0x08)
                  + ")!\n";
        }
        if (
                readBytes<4, u32, Endianness::BIG>(buffer + offset + 0x0C)
             != 0x666D7420) {
            message +=
                    "WARNING: missing 'fmt ' magic (@"
                  + std::to_string(offset + 0x0C)
                  + ")!\n";
        }
        if (
                readBytes<4, u32, Endianness::BIG>(buffer + offset + 0x24)
             != 0x64617461) {
            message +=
                    "WARNING: missing data magic (@"
                  + std::to_string(offset + 0x24)
                  + ")!\n";
        }
        if (readBytes<4, u32>(buffer + offset + 0x10) != 16) {
            message +=
                    "WARNING: subchunk 1 size (@"
                  + std::to_string(offset + 0x10)
                  + ") is not 16!\n";
        }
        if (readBytes<2, u16>(buffer + offset + 0x14) != 1) {
            message +=
                    "WARNING: format (@"
                  + std::to_string(offset + 0x14)
                  + ") is not 1 (LPCM)!\n";
        }
        if (info.frameSize != (info.bitDepth / 8) * info.channelCount) {
            message +=  
                    "WARNING: frame size field (@"
                  + std::to_string(offset + 0x20)
                  + ") is misconfigured!\n";
        }
        if (
                readBytes<4, u32>(buffer + offset + 0x1C)
             != info.sampleRate * info.frameSize) {
            message +=
                    "WARNING: byte rate field (@"
                  + std::to_string(offset + 0x1C)
                  + ") is misconfigured!\n";
        }
        if (
                readBytes<4, u32>(buffer + offset + 0x04) 
             != info.dataSize + 0x24) { 
            message += 
                    "WARNING: mismatched chunk size (@"
                  + std::to_string(offset + 0x04)
                  + ") and data size (@"
                  + std::to_string(offset + 0x28)
                  + ")!\n";
        }
        if (info.bitDepth != 16) {
            message += 
                    "WARNING: bit depth (@"
                  + std::to_string(offset + 0x22)
                  + ") is not 16!\n";
        }

        return info;
    }
}
