#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include "byte.hpp"

namespace dspAdpcm {
    static constexpr std::array<s16, 32> afcCoefficients {
            0, 0,         2048, 0,      0, 2048,      1024, 1024,
            4096, -2048,  3584, -1536,  3072, -1024,  4608, -2560,
            4200, -2248,  4800, -2300,  5120, -3072,  2048, -2048,
            1024, -1024,  -1024, 1024,  -1024, 0,     -2048, 0
    };

    template <typename AddressType, typename CoefficientsType>
    s16 toLpcm(
            const AddressType address,
            const bool readTopNibble,
            const CoefficientsType coefficients,
            const u8_fast predictor,
            const u8_fast scale,
            s16& history1,
            s16& history2) {
        s64_fast sample {(*address >> (readTopNibble ? 4 : 0)) & 0x0F}; 
        sample = sample & 0x08 ? sample - 0x10 : sample; 
        sample = 
                ((sample << (scale + 11))
              + coefficients[predictor * 2    ] * history1
              + coefficients[predictor * 2 + 1] * history2) >> 11; 
        sample = std::min(static_cast<s64_fast>(INT16_MAX), sample); 
        sample = std::max(static_cast<s64_fast>(INT16_MIN), sample);

        history2 = history1;
        history1 = sample;

        return sample;
    }
}
