cmake_minimum_required(VERSION 3.8.2)

project(riptendo
        VERSION 0.0.2
        LANGUAGES CXX)

add_executable(riptendo
        main.cpp
        ast.hpp
        byte.hpp
        dsp-adpcm.hpp
        file.hpp
        wav.hpp)
target_compile_features(riptendo
        PUBLIC cxx_std_11)
set_target_properties(riptendo PROPERTIES
        CXX_EXTENSIONS OFF)

install(
        TARGETS riptendo
        RUNTIME
        DESTINATION /usr/bin
        CONFIGURATIONS Debug Release)
