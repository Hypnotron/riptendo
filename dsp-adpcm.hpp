#pragma once
#include <algorithm>
#include <array>
#include <cmath>
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
    s16 sampleToLpcm(
            const AddressType address,
            const bool readTopNibble,
            const CoefficientsType coefficients,
            const u8_fast predictor,
            const u8_fast scale,
            const bool updateHistory,
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

        if (updateHistory) {
            history2 = history1;
            history1 = sample;
        }

        return sample;
    }
    template <typename AddressType, typename CoefficientsType>
    u8 sampleFromLpcm(
            const AddressType address,
            const CoefficientsType coefficients,
            const u8_fast predictor,
            const u8_fast scale,
            const bool updateHistory,
            s16& history1,
            s16& history2) {
        s8 adpcmSample {
                ((*address << 11)
              - coefficients[predictor * 2    ] * history1
              - coefficients[predictor * 2 + 1] * history2)
             >> (scale + 11)};
        adpcmSample = std::min(static_cast<s8>(0x7), adpcmSample);
        adpcmSample = std::max(static_cast<s8>(-0x8), adpcmSample);
        adpcmSample =
                adpcmSample < 0 ? adpcmSample + 0x10 : adpcmSample;

        if (updateHistory) {
            history2 = history1;
            history1 = *address;
        }

        return adpcmSample;
    }
    template <
            typename InputType,
            typename OutputType,
            typename CoefficientsType>
    void blockFromLpcm(
            const InputType input,
            OutputType& output,
            const CoefficientsType coefficients,
            //TODO: updateHistory parameter
            s16& history1,
            s16& history2) {
        u32_fast bestError {UINT32_MAX};
        u8_fast bestPredictor {0};
        u8_fast bestScale {0};
        //TODO: calculate best predictor and scale without brute force
        for (u8_fast predictor {0}; predictor < 16; ++predictor) {
            for (u8_fast scale {0}; scale < 16; ++scale) {
                u32_fast error {0};
                s16 temporaryHistory1 {history1};
                s16 temporaryHistory2 {history2};
                for (
                        InputType lpcmSampleOffset {input};
                        lpcmSampleOffset - input < 16;
                        ++lpcmSampleOffset) {
                    u8 adpcmSample {sampleFromLpcm(
                            lpcmSampleOffset,
                            coefficients,
                            predictor,
                            scale,
                            false,
                            temporaryHistory1,
                            temporaryHistory2)};
                    error += std::abs(
                            *lpcmSampleOffset
                          - sampleToLpcm(
                                    &adpcmSample,
                                    false,
                                    coefficients,
                                    predictor,
                                    scale,
                                    true,
                                    temporaryHistory1,
                                    temporaryHistory2));
                }
                if (error < bestError) {
                    bestPredictor = predictor;
                    bestScale = scale;
                    bestError = error;
                }
            }
        }
        *output++ = (bestScale << 4) | bestPredictor;
        bool writeTopNibble {true};
        for (
                InputType lpcmSampleOffset {input};
                lpcmSampleOffset - input < 16;
                ++lpcmSampleOffset) {
            u8 adpcmSample {sampleFromLpcm(
                    lpcmSampleOffset,
                    coefficients,
                    bestPredictor,
                    bestScale,
                    false,
                    history1,
                    history2)};
            //Update history samples:
            static_cast<void>(sampleToLpcm(
                    &adpcmSample,
                    false,
                    coefficients,
                    bestPredictor,
                    bestScale,
                    true,
                    history1,
                    history2));
            if (writeTopNibble) {
                *output = adpcmSample << 4;
            }
            else {
                *output++ |= adpcmSample & 0x0F;
            }
            writeTopNibble = !writeTopNibble;
        }
    }
}
