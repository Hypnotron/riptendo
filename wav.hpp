#pragma once
#include "file.hpp"

class WavFile : public File {
    private:

    public:
        WavFile() {}
        WavFile(std::string filename)
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
