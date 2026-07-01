#pragma once

#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

/**
    Stereo implementation of the Airwindows Mackity DSP algorithm.
    Emulates the crunchy, warm input gain stage of a vintage Mackie 1202 mixer.
*/
class MackityProcessor
{
public:
    MackityProcessor()
    {
        reset();
    }

    void prepareToPlay (double newSampleRate)
    {
        sampleRate = newSampleRate;
        reset();
    }

    void reset()
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            iirSampleA[channel] = 0.0;
            iirSampleB[channel] = 0.0;
            
            for (int x = 0; x < 11; ++x)
            {
                biquadA[channel][x] = 0.0;
                biquadB[channel][x] = 0.0;
            }
            
            // Initialize fpd deterministically to prevent static initializer load issues
            fpd[channel] = (uint32_t)(17 + channel * 1000);
            while (fpd[channel] < 16386)
            {
                fpd[channel] ^= fpd[channel] << 13;
                fpd[channel] ^= fpd[channel] >> 17;
                fpd[channel] ^= fpd[channel] << 5;
            }
        }
    }

    void processBlock (juce::AudioBuffer<float>& buffer, float driveParam, float outputParam, bool isEnabled)
    {
        if (!isEnabled)
            return;

        double sampleRateVal = sampleRate > 0.0 ? sampleRate : 44100.0;
        double overallscale = sampleRateVal / 44100.0;

        double inTrim = driveParam * 10.0;
        inTrim *= inTrim;
        double outPad = outputParam;

        double iirAmountA = 0.001860867 / overallscale;
        double iirAmountB = 0.000287496 / overallscale;

        // Cap to prevent Nyquist limit errors in tangent functions
        double biquadCutoff = 19160.0 / sampleRateVal;
        if (biquadCutoff > 0.49) 
            biquadCutoff = 0.49;

        double biquadA0 = biquadCutoff;
        double biquadB0 = biquadCutoff;
        double biquadA1 = 0.431684981684982;
        double biquadB1 = 1.1582298;

        // Biquad A Coefficients
        double K = std::tan (juce::MathConstants<double>::pi * biquadA0);
        double norm = 1.0 / (1.0 + K / biquadA1 + K * K);
        double coeffA2 = K * K * norm;
        double coeffA3 = 2.0 * coeffA2;
        double coeffA4 = coeffA2;
        double coeffA5 = 2.0 * (K * K - 1.0) * norm;
        double coeffA6 = (1.0 - K / biquadA1 + K * K) * norm;

        // Biquad B Coefficients
        K = std::tan (juce::MathConstants<double>::pi * biquadB0);
        norm = 1.0 / (1.0 + K / biquadB1 + K * K);
        double coeffB2 = K * K * norm;
        double coeffB3 = 2.0 * coeffB2;
        double coeffB4 = coeffB2;
        double coeffB5 = 2.0 * (K * K - 1.0) * norm;
        double coeffB6 = (1.0 - K / biquadB1 + K * K) * norm;

        int numChannels = buffer.getNumChannels();
        int numSamples = buffer.getNumSamples();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            if (channel >= 2) 
                break; // Stereo processing only

            float* channelData = buffer.getWritePointer (channel);
            
            auto& chanBiquadA = biquadA[channel];
            auto& chanBiquadB = biquadB[channel];
            double& chanIirSampleA = iirSampleA[channel];
            double& chanIirSampleB = iirSampleB[channel];
            uint32_t& chanFpd = fpd[channel];

            for (int i = 0; i < numSamples; ++i)
            {
                double inputSample = (double)channelData[i];

                // Check for denormals
                if (std::abs (inputSample) < 1.18e-23) 
                    inputSample = chanFpd * 1.18e-17;

                // IIR filter stage A (Low Frequency Roll-off)
                if (std::abs (chanIirSampleA) < 1.18e-37) 
                    chanIirSampleA = 0.0;
                chanIirSampleA = (chanIirSampleA * (1.0 - iirAmountA)) + (inputSample * iirAmountA);
                inputSample -= chanIirSampleA;

                // Input Trim
                if (inTrim != 1.0) 
                    inputSample *= inTrim;

                // Biquad Filter stage A
                double outSample = coeffA2 * inputSample 
                                 + coeffA3 * chanBiquadA[7] 
                                 + coeffA4 * chanBiquadA[8] 
                                 - coeffA5 * chanBiquadA[9] 
                                 - coeffA6 * chanBiquadA[10];
                                 
                chanBiquadA[8] = chanBiquadA[7]; 
                chanBiquadA[7] = inputSample; 
                inputSample = outSample; 
                chanBiquadA[10] = chanBiquadA[9]; 
                chanBiquadA[9] = inputSample;

                // Hard Limit / Saturation (Mackie Mixer overdrive model)
                if (inputSample > 1.0) 
                    inputSample = 1.0;
                if (inputSample < -1.0) 
                    inputSample = -1.0;
                
                // Waveshaping: subtraction of the quintic term
                inputSample -= std::pow (inputSample, 5) * 0.1768;

                // Biquad Filter stage B
                outSample = coeffB2 * inputSample 
                          + coeffB3 * chanBiquadB[7] 
                          + coeffB4 * chanBiquadB[8] 
                          - coeffB5 * chanBiquadB[9] 
                          - coeffB6 * chanBiquadB[10];
                          
                chanBiquadB[8] = chanBiquadB[7]; 
                chanBiquadB[7] = inputSample; 
                inputSample = outSample; 
                chanBiquadB[10] = chanBiquadB[9]; 
                chanBiquadB[9] = inputSample;

                // IIR filter stage B
                if (std::abs (chanIirSampleB) < 1.18e-37) 
                    chanIirSampleB = 0.0;
                chanIirSampleB = (chanIirSampleB * (1.0 - iirAmountB)) + (inputSample * iirAmountB);
                inputSample -= chanIirSampleB;

                // Output Pad
                if (outPad != 1.0) 
                    inputSample *= outPad;

                // 32-bit floating point dither
                int expon; 
                std::frexp (inputSample, &expon);
                
                chanFpd ^= chanFpd << 13; 
                chanFpd ^= chanFpd >> 17; 
                chanFpd ^= chanFpd << 5;
                
                // Use ldexp and clamp to prevent potential NaN/overflow or performance issues in std::pow
                int shift = std::clamp (expon + 62, -1000, 1000);
                inputSample += ((double (chanFpd) - 2147483647.0) * 5.5e-36 * std::ldexp (1.0, shift));

                channelData[i] = (float)inputSample;
            }
        }
    }

private:
    double sampleRate = 44100.0;
    double iirSampleA[2] = { 0.0 };
    double iirSampleB[2] = { 0.0 };
    double biquadA[2][11] = { { 0.0 } };
    double biquadB[2][11] = { { 0.0 } };
    uint32_t fpd[2] = { 1, 1 };
};
