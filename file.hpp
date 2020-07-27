#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "byte.hpp"

class File {
    private:

    public:
        std::string filename;
        std::vector<u8> buffer;
        size_t start {0};
        size_t end;

        File () {}
        File(const std::string& filename)
              : filename{filename} {
            std::ifstream file {filename.c_str(), std::ios_base::binary};
            auto start {file.tellg()};
            file.seekg(0, std::ios_base::end);
            buffer.resize(file.tellg() - start);
            end = buffer.size() + 1;

            file.seekg(0, std::ios_base::beg);
            file.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        }

        void flush() const {
            std::ofstream file {filename.c_str(), std::ios_base::binary};
            file.write(
                    reinterpret_cast<const char*>(buffer.data()), 
                    buffer.size());
        }

        virtual void toWav(File& dest) const = 0;
        virtual void fromWav(const File& source) = 0;

        virtual ~File() {}
};
