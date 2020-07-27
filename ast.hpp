#pragma once
#include <vector>
#include "byte.hpp"
#include "dsp-adpcm.hpp"
#include "file.hpp"

//TODO: assertions
class AstFile : public File {
    private:

    public:
        AstFile() {}
        AstFile(std::string filename)
              : File(filename) {
        }

        void toWav(File& dest) const override {
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
            dest.buffer.resize(
                    sampleCount * (bitDepth / 8) * channelCount
                  + 0x2C);
            std::vector<u8>::iterator output {
                    dest.buffer.begin() + dest.start + 0x2C};

            std::vector<s16> history1(channelCount, 0);
            std::vector<s16> history2(channelCount, 0);

            //Advance beyond main (STRM) header:
            u32_fast blockOffset {0x40};
            s64_fast samplesRemaining {sampleCount};
            while (samplesRemaining > 0) {
                const u32 blockSize {readBytes<4, u32, Endianness::BIG>(
                        buffer.begin() + start + blockOffset + 0x04)};
                if (isPcm) {
                    //Advance beyond BLCK header: 
                    blockOffset += 0x20;
                    for (
                            u32 sampleOffset {0};
                            sampleOffset < blockSize;
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
                            frameOffset < blockSize;
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
                                sampleOffset < 18; 
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
                                                //TODO: negate
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
        }

        void fromWav(const File& source) override {
            //TODO: conversion from wav
        }
};
