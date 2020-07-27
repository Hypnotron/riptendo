#pragma once
#include <string>
#include <vector>
#include "byte.hpp"
#include "dsp-adpcm.hpp"
#include "file.hpp"

class AstFile : public File {
    private:

    public:
        AstFile() {}
        AstFile(std::string filename)
              : File(filename) {
        }

        std::string toWav(File& dest) const override {
            if (start + 0x40 > end) {
                return "Missing AST header (out of space)!\n";
            }

            const bool isPcm = readBytes<2, u16, Endianness::BIG>(
                    buffer.begin() + start + 0x08); 
            const u16 bitDepth {readBytes<2, u16, Endianness::BIG>(
                    buffer.begin() + start + 0x0A)};
            const u16 channelCount {readBytes<2, u16, Endianness::BIG>(
                    buffer.begin() + start + 0x0C)};
            const u32 sampleRate {readBytes<4, u32, Endianness::BIG>(
                    buffer.begin() + start + 0x10)};
            const u32 sampleCount {readBytes<4, u32, Endianness::BIG>(
                    buffer.begin() + start + 0x14)};
            //TODO: AST looping

            //Accomodate for 44-byte wav header:
            dest.end = 
                    dest.start
                  + sampleCount * (bitDepth / 8) * channelCount
                  + 0x2C;
            dest.buffer.resize(std::max(dest.buffer.size(), dest.end));
            std::vector<u8>::iterator output {
                    dest.buffer.begin() + dest.start};
            //                                       R I F F
            writeBytes<4, Endianness::BIG>(output, 0x52494646); output += 4;
            writeBytes<4>(output, dest.end - 8);      output += 4;
            //                                       W A V E
            writeBytes<4, Endianness::BIG>(output, 0x57415645); output += 4;
            //                                       f m t <space>
            writeBytes<4, Endianness::BIG>(output, 0x666D7420); output += 4; 
            //Subchunk 1 size (always 16 for LPCM):
            writeBytes<4>(output, 16);                          output += 4;
            //Audio format (always 1 for LPCM):
            writeBytes<2>(output, 1);                           output += 2;
            writeBytes<2>(output, channelCount);                output += 2;
            writeBytes<4>(output, sampleRate);                  output += 4;
            writeBytes<4>(output, 
                    sampleRate 
                  * (bitDepth / 8) 
                  * channelCount);                              output += 4;
            writeBytes<2>(output,
                    (bitDepth / 8) * channelCount);             output += 2;
            writeBytes<2>(output, bitDepth);                    output += 2;
            //                                       d a t a
            writeBytes<4, Endianness::BIG>(output, 0x64617461); output += 4;
            writeBytes<4>(output, dest.end - 0x2C);   output += 4;

            std::vector<s16> history1(channelCount, 0);
            std::vector<s16> history2(channelCount, 0);

            //Advance beyond main (STRM) header:
            u32_fast blockOffset {0x40};
            s64_fast samplesRemaining {sampleCount};
            while (samplesRemaining > 0) {
                if (start + blockOffset + 0x20 > end) {
                    return "Sample count in header is"
                           " too high (out of space)!\n";
                }
                const u32 blockSize {readBytes<4, u32, Endianness::BIG>(
                        buffer.begin() + start + blockOffset + 0x04)};
                if (
                        start 
                      + blockOffset 
                      + 0x20 
                      + blockSize * channelCount 
                      > end) {
                    return 
                            std::string("Block size ") 
                          + std::to_string(blockSize) 
                          + std::string(" is too large (out of space)!\n");
                }
                if (isPcm) {
                    //Advance beyond BLCK header: 
                    blockOffset += 0x20;
                    for (
                            u32 sampleOffset {0};
                            sampleOffset < blockSize && samplesRemaining > 0;
                            sampleOffset += bitDepth / 8, --samplesRemaining) {
                        for (
                                u16 channelIndex {0};
                                channelIndex < channelCount;
                                ++channelIndex) {
                            writeBytes(
                                    bitDepth / 8,
                                    Endianness::LITTLE,
                                    output,
                                    readBytes<u64>(
                                            bitDepth / 8,
                                            Endianness::BIG,
                                            buffer.begin()
                                          + start
                                          + blockOffset
                                          + blockSize * channelIndex
                                          + sampleOffset));
                            output += bitDepth / 8;
                        }
                    }
                }
                else {
                    for (
                            u16 channelIndex {0};
                            channelIndex < channelCount;
                            ++channelIndex) {
                        //TODO: determine actual history sample order 
                        /*
                        history2[channelIndex] = toSigned(readBytes<
                                2, 
                                u16,
                                Endianness::BIG>(
                                buffer.begin() + start + blockOffset + 0x08
                              + channelIndex * 0x04));
                        history1[channelIndex] = toSigned(readBytes<
                                2, 
                                u16,
                                Endianness::BIG>(
                                buffer.begin() + start + blockOffset + 0x08
                              + channelIndex * 0x04 + 0x02));
                        */
                    }
                    //Advance beyond BLCK header: 
                    blockOffset += 0x20;
                    for (
                            u32 frameOffset {0};
                            frameOffset + 9 <= blockSize;
                            frameOffset += 9) {
                        std::vector<u8_fast> predictors(channelCount);
                        std::vector<u8_fast> scales(channelCount);
                        for (
                                u16 channelIndex {0};
                                channelIndex < channelCount;
                                ++channelIndex) {
                            predictors[channelIndex] = buffer[
                                    start
                                  + blockOffset
                                  + blockSize * channelIndex
                                  + frameOffset] & 0x0F;
                            scales[channelIndex] = (buffer[
                                    start
                                  + blockOffset
                                  + blockSize * channelIndex
                                  + frameOffset] >> 4) & 0x0F;
                        }

                        for (
                                u32 sampleOffset {2}; 
                                sampleOffset < 18 && samplesRemaining > 0; 
                                ++sampleOffset, --samplesRemaining) {
                            for (
                                    u16 channelIndex {0};
                                    channelIndex < channelCount;
                                    ++channelIndex) {
                                writeBytes<2>(
                                        output,
                                        dspAdpcm::toLpcm(
                                                buffer.begin()
                                              + start
                                              + blockOffset
                                              + blockSize * channelIndex
                                              + frameOffset
                                              + sampleOffset / 2,
                                                !(sampleOffset & 0x01),
                                                dspAdpcm::afcCoefficients
                                                        .begin(),
                                                predictors[channelIndex],
                                                scales[channelIndex],
                                                history1[channelIndex],
                                                history2[channelIndex]));
                                output += 2; 
                            }
                        }
                    }
                }
                blockOffset += blockSize * channelCount;
            }
            return "";
        }

        std::string fromWav(const File& source) override {
            //TODO: conversion from wav
            return "";
        }
};
