#pragma once
#include <string>
#include <vector>
#include "byte.hpp"
#include "dsp-adpcm.hpp"
#include "file.hpp"
#include "wav.hpp"

class AstFile : public File {
    private:

    public:
        struct {
            u16 codec {1};
            u32 loopStart {0};
            u32 loopEnd {0};
        } encoderProperties;

        AstFile() {}
        AstFile(const std::string& filename)
              : File(filename) {
        }

        std::string toWav(File& dest) const override {
            //TODO: downmix to 16-bit
            std::string message {""};

            if (start + 0x40 > end) {
                message += "ERROR (out of space): missing AST header!\n";
                return message;
            }
            const u32 strmMagic {readBytes<4, u32, Endianness::BIG>(
                    buffer.begin() + start)};
            const u32 dataSize {readBytes<4, u32, Endianness::BIG>(
                    buffer.begin() + start + 0x04)};
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
            if (strmMagic != 0x5354524D) {
                message += 
                        "WARNING: missing STRM file magic (@"
                      + std::to_string(start)
                      + ")!\n";
            }
            if (start + 0x40 + dataSize > end) {
                message += 
                        "WARNING: data size field in header (@"
                      + std::to_string(start + 0x04)
                      + ") is too large!\n";
            }

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
            writeBytes<4>(output, dest.end - dest.start - 8);   output += 4;
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
            writeBytes<4>(output, dest.end - dest.start - 0x2C);output += 4;

            std::vector<s16> history1(channelCount, 0);
            std::vector<s16> history2(channelCount, 0);

            //Advance beyond main (STRM) header:
            u32_fast blockOffset {0x40};
            s64_fast samplesRemaining {sampleCount};
            while (samplesRemaining > 0) {
                if (start + blockOffset + 0x20 > end) {
                    message +=
                            "ERROR (out of space): sample count in header (@" 
                          + std::to_string(start + 0x14)
                          + ") is too high!\n";
                    return message; 
                }
                const u32 blckMagic {readBytes<4, u32, Endianness::BIG>(
                        buffer.begin() + start + blockOffset)};
                const u32 blockSize {readBytes<4, u32, Endianness::BIG>(
                        buffer.begin() + start + blockOffset + 0x04)};
                if (blckMagic != 0x424C434B) {
                    message += 
                            "WARNING: missing BLCK magic (@"
                          + std::to_string(start + blockOffset)
                          + ")!\n";
                }
                if (
                        start 
                      + blockOffset 
                      + 0x20 
                      + blockSize * channelCount 
                      > end) {
                    message += 
                            "ERROR (out of space): block size " 
                          + std::to_string(blockSize) 
                          + " (@"
                          + std::to_string(start + blockOffset + 0x04)
                          + ") is too large!\n";
                    return message;
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
                                    ++channelIndex, output += 2) {
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
                            }
                        }
                    }
                }
                blockOffset += blockSize * channelCount;
            }
            if (blockOffset > dataSize + 0x40) {
                message += 
                        "WARNING: data size field in header (@"
                      + std::to_string(start + 0x04)
                      + ") is too small!\n";
            }  
            if (start + blockOffset < end) {
                message +=
                        "WARNING: skipped the final "
                      + std::to_string(end - blockOffset)
                      + " bytes (sample count (@"
                      + std::to_string(start + 0x14)
                      + ") may be too low)!\n";
            }
            return message;
        }
        std::string fromWav(const File& source) override {
            std::string message {""};
                
            if (source.start + 0x2C > source.end) {
                message += "ERROR (out of space): missing WAV header!\n";
                return message;
            }
            wav::Info info {wav::parseHeader(
                    source.buffer.begin(), 
                    source.start, 
                    message)};
            end =
                    (start
                  + 0x40
                  //size of BLCK headers:
                  + (info.dataSize / (10080 * info.channelCount) + 1) * 0x20
                  + info.dataSize
                  //pad to nearest multiple of 32:
                  + 0x1F) & ~(0x1F);
            buffer.resize(std::max(buffer.size(), end));
            std::vector<u8>::iterator output {buffer.begin() + start};

            //                                       S T R M
            writeBytes<4, Endianness::BIG>(output, 0x5354524D); output += 4;
            writeBytes<4, Endianness::BIG>(
                    output, 
                    end - start - 0x40);                        output += 4;
            writeBytes<2, Endianness::BIG>(
                    output, 
                    encoderProperties.codec);                   output += 2;
            writeBytes<2, Endianness::BIG>(
                    output, 
                    info.bitDepth);                             output += 2;
            writeBytes<2, Endianness::BIG>(
                    output, 
                    info.channelCount);                         output += 2;
            //unknown:
            writeBytes<2, Endianness::BIG>(output, 0xFFFF);     output += 2;
            writeBytes<4, Endianness::BIG>(
                    output, 
                    info.sampleRate);                           output += 4;
            writeBytes<4, Endianness::BIG>(
                    output, 
                    info.dataSize / info.frameSize);            output += 4;
            writeBytes<4, Endianness::BIG>(
                    output, 
                    encoderProperties.loopStart);               output += 4;
            writeBytes<4, Endianness::BIG>(
                    output, 
                    encoderProperties.loopEnd);                 output += 4;
            writeBytes<4, Endianness::BIG>(
                    output,
                    info.dataSize / info.channelCount < 10080
                  ? info.dataSize / info.channelCount
                  : 10080);                                     output += 4;
            //unknown:
            writeBytes<4, Endianness::BIG>(output, 0);          output += 4;
            //unknown:
            writeBytes<4, Endianness::BIG>(output, 0x7F000000); output += 4;
            //unknown:
            for (u8_fast i {0x14}; i > 0; --i) {
                *output++ = 0;
            }

            //Advance beyond main (STRM) header:
            u32_fast wavBlockOffset {0x2C};
            for (
                    s64_fast blocksRemaining {
                            info.dataSize / (10080 * info.channelCount) + 1};
                    blocksRemaining > 0 
                 && source.start + wavBlockOffset < source.end;
                    --blocksRemaining) {
                const u32 blockSize {
                        blocksRemaining == 1
                      ? ((info.dataSize / info.channelCount) % 10080 + 0x1F) 
                      & ~(0x1F) 
                      : 10080};
                writeBytes<4, Endianness::BIG>(
                        output, 
                        0x424C434B);                            output += 4; 
                writeBytes<4, Endianness::BIG>(
                        output,
                        blockSize);                             output += 4;
                for (u8_fast i {0x18}; i > 0; --i) {
                    *output++ = 0;
                }
                for (
                        u16 channelIndex {0};
                        channelIndex < info.channelCount;
                        ++channelIndex) {
                    switch (encoderProperties.codec) {
                    //AFC ADPCM:
                    case 0:
                        //TODO: AST AFC ADPCM encoding
                    break;

                    //LPCM:
                    case 1:
                        for (
                                u32 wavSampleOffset =
                                        channelIndex 
                                      * (info.bitDepth / 8);
                                wavSampleOffset < 
                                        blockSize 
                                      * info.channelCount;
                                wavSampleOffset += 
                                        info.channelCount
                                      * (info.bitDepth / 8),
                                output += 2) {
                            writeBytes<2, Endianness::BIG>(
                                    output,
                                    source.start 
                                  + wavBlockOffset 
                                  + wavSampleOffset
                                  < source.end
                                  ? readBytes<u64>(
                                            info.bitDepth / 8,
                                            Endianness::LITTLE,
                                            source.buffer.begin()
                                          + source.start
                                          + wavBlockOffset
                                          + wavSampleOffset)
                                  : 0);
                        }
                    break;

                    default:
                        message += 
                                "ERROR: unknown codec type "
                              + std::to_string(encoderProperties.codec)
                              + "!\n";
                        return message;
                    }
                }
                wavBlockOffset += blockSize * info.channelCount;
            }
            return message; 
        }
};
