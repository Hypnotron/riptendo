#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include "file.hpp"
#include "wav.hpp"
#include "ast.hpp"

void printUsageAndExit(const char* const executable) {
    std::cerr 
            << "usage: " << executable << " <format> [decode|encode]"
            << " <input-file> <output-file>\n"; 
    exit(1);
}

int main(int argc, char* argv[]) {
    //Print version:
    std::cerr << "riptendo version 0.0.2\n"; 

    //Validate arguments:
    if (
            argc < 5
         || (std::strcmp(argv[2], "decode") 
         &&  std::strcmp(argv[2], "encode"))) {
        printUsageAndExit(argv[0]);
    }

    //Set decode/encode mode:
    const bool decoding = !std::strcmp(argv[2], "decode");

    //Create file objects:
    File* inputFile {nullptr};
    File* outputFile {nullptr};
    File*& wavFile {decoding ? outputFile : inputFile};
    File*& nintendoFile {decoding ? inputFile : outputFile};

    //Set filenames: 
    const std::string wavFilename {decoding ? argv[4] : argv[3]};
    const std::string nintendoFilename {decoding ? argv[3] : argv[4]};

    //Construct .wav file:
    wavFile = new WavFile(wavFilename);

    //Construct Nintendo file:
    if (!std::strcmp(argv[1], "ast")) {
        nintendoFile = new AstFile(nintendoFilename);
    }
    else {
        std::cerr << "unsupported format " << argv[1] << '\n'; 
        exit(2);
    }

    //Perform conversion and flush:
    if (decoding) {
        nintendoFile->toWav(*wavFile);
    }
    else {
        nintendoFile->fromWav(*wavFile);
    }
    outputFile->flush();

    //Cleanup:
    delete inputFile;
    delete outputFile;
    return 0;
}
