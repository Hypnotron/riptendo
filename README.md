# riptendo
A C++11 program for ripping Nintendo audio

#
#
#
#

# Building
#### Requirements
+ CMake 3.8.2+
+ A C++11 capable compiler


    mkdir build && cd build
    cmake ..
        -DCMAKE_BUILD_TYPE=Release \
        -DMYVARIABLE=MYVALUE #optional additional variables
Then follow the platform-specific instructions:
#### Unix-like:
    make
    sudo make install
#### Windows:
Open the Visual Studio project and compile with F7.

#
#
#
#

# Usage
### .ast
##### Decoding:
    riptendo ast decode <input-file.ast> <output-file.wav>
##### Encoding:
    riptendo ast encode <input-file.wav> <output-file.ast> \
            <codec> <loop start> <loop end>

    Codec:          0 for AFC-ADPCM. 1 for LPCM

    Loop Start:     the sample at which the loop should occur
                    (starting at 0, where one sample includes all channels)

    Loop End:       the sample at which the loop should finish
                    (starting at 0, where one sample includes all channels)
