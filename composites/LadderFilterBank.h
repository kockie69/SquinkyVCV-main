#pragma once
#include "LadderFilter.h"
#include "PeakDetector.h"
#include "SqPort.h"
#include "SqStream.h"

template <typename T>
class LadderFilterBank {
public:
    enum class Modes {
        normal,    // mono, poly. R out == L out
        stereo,    // in L -> out L, in R -> out R
        leftOnly,  // in L -> out L, out R
        rightOnly  // in R -> out L, out R
    };
    void stepn(float sampleTime, int numChannels,
               SqInput& fc1Input, SqInput& fc2Input, SqInput& qInput, SqInput& driveInput, SqInput& edgeInput, SqInput& slopeInput,
               float fcParam, float fc1TrimParam, float fc2TrimParam,
               T volume,
               float qParam, float qTrimParam, float makeupGainParam,
               typename LadderFilter<T>::Types type, typename LadderFilter<T>::Voicing voicing,
               float driveParam, float driveTrim,
               float edgeParam, float edgeTrim,
               float slopeParam, float slopeTrim,
               float spread);

    /**
     * inputForChannel0 and 1 are optional.
     * They will be null except for special modes.
     */
    void step(int numChannels, Modes mode,
              SqInput& audioInput, SqOutput& audioOutput, SqOutput& audioOutputR,
              SqInput* inputForChannel0, SqInput* inputForChannel1,
              PeakDetector& peak, bool poly);

    void _dump(int channel, const std::string& label) {
        SqStream s;
        s.add(label);
        s.add("[");
        s.add(channel);
        s.add("] ");
        filterL[channel]._dump(s.str());
        filterR[channel]._dump(s.str());
    }
    const LadderFilter<T>& get(int channel) {
        //return filters[channel];
        return filterL[channel];
    }

private:
    LadderFilter<T> filterL[16];
    LadderFilter<T> filterR[16];

    std::shared_ptr<LookupTableParams<T>> expLookup = ObjectCache<T>::getExp2();  // Do we need more precision?
    AudioMath::ScaleFun<float> scaleGain = AudioMath::makeLinearScaler<float>(0, 1);
    std::shared_ptr<LookupTableParams<float>> audioTaper = {ObjectCache<float>::getAudioTaper()};

    AudioMath::ScaleFun<float> scaleFc = AudioMath::makeScalerWithBipolarAudioTrim(-5, 5);
    AudioMath::ScaleFun<float> scaleQ = AudioMath::makeScalerWithBipolarAudioTrim(0, 4);
    AudioMath::ScaleFun<float> scaleSlope = AudioMath::makeScalerWithBipolarAudioTrim(0, 3);
    AudioMath::ScaleFun<float> scaleEdge = AudioMath::makeScalerWithBipolarAudioTrim(0, 1);
};

template <typename T>
inline void LadderFilterBank<T>::stepn(float sampleTime, int numChannels,
                                       SqInput& fc1Input, SqInput& fc2Input, SqInput& qInput, SqInput& driveInput, SqInput& edgeInput, SqInput& slopeInput,
                                       float fcParam, float fc1TrimParam, float fc2TrimParam,
                                       T volume,
                                       float qParam, float qTrimParam, float makeupGainParam,
                                       typename LadderFilter<T>::Types type, typename LadderFilter<T>::Voicing voicing,
                                       float driveParam, float driveTrimParam,
                                       float edgeParam, float edgeTrim,
                                       float slopeParam, float slopeTrim,
                                       float spreadParam) {

    for (int channel = 0; channel < numChannels; ++channel) {
        LadderFilter<T>& filt = filterL[channel];
        LadderFilter<T>& filtR = filterR[channel];

        filt.setType(type);
        filtR.setType(type);
        filt.setVoicing(voicing);
        filtR.setVoicing(voicing);
        filt.setVolume(volume);
        filtR.setVolume(volume);

        // filter Fc calc
        {
            T fcClipped = 0;
            T freqCV1 = scaleFc(
                fc1Input.getPolyVoltage(channel),
                fcParam,
                fc1TrimParam);
            T freqCV2 = scaleFc(
                fc2Input.getPolyVoltage(channel),
                0,
                fc2TrimParam);  // note: test second inputs
            T freqCV = freqCV1 + freqCV2 + 6;
            const T fc = LookupTable<T>::lookup(*expLookup, freqCV, true) * 10;
            const T normFc = fc * sampleTime;

            fcClipped = std::min(normFc, T(.48));
            fcClipped = std::max(fcClipped, T(.0000001));
            filt.setNormalizedFc(fcClipped);
            filtR.setNormalizedFc(fcClipped);
        }
        {
            T res = scaleQ(
                qInput.getPolyVoltage(0),
                qParam,
                qTrimParam);
            const T qMiddle = 2.8;
            res = (res < 2) ? (res * qMiddle / 2) : .5 * (res - 2) * (4 - qMiddle) + qMiddle;

            if (res < 0 || res > 4) fprintf(stderr, "res out of bounds %f\n", res);

            const T bAmt = makeupGainParam;
            const T makeupGain = 1 + bAmt * (res);

            filt.setFeedback(res);
            filtR.setFeedback(res);
            filt.setBassMakeupGain(makeupGain);
            filtR.setBassMakeupGain(makeupGain);
        }
        {
            float gainInput = scaleGain(
                driveInput.getPolyVoltage(channel),
                driveParam,
                driveTrimParam);

            T gain = T(.15) + 4 * LookupTable<float>::lookup(*audioTaper, gainInput, false);
            filt.setGain(gain);
            filtR.setGain(gain);
        }
        {
            const float edge = scaleEdge(
                edgeInput.getPolyVoltage(channel),
                edgeParam,
                edgeTrim);
            filt.setEdge(edge);
            filtR.setEdge(edge);
        }
        {
            T slope = scaleSlope(
                slopeInput.getPolyVoltage(channel),
                slopeParam,
                slopeTrim);
            filt.setSlope(slope);
            filtR.setSlope(slope);
        }
        filt.setFreqSpread(spreadParam);
        filtR.setFreqSpread(spreadParam);
    }
}

template <typename T>
inline void LadderFilterBank<T>::step(int numChannels, Modes mode,
                                      SqInput& audioInput, SqOutput& audioOutput, SqOutput& audioOutputR,
                                      SqInput* inputForChannel0, SqInput* inputForChannel1,
                                      PeakDetector& peak, bool poly) {

    for (int channel = 0; channel < numChannels; ++channel) {
        LadderFilter<T>& filt = filterL[channel];

        float input = audioInput.getVoltage(channel);
        if (!poly) {
            switch (mode) {
                case Modes::stereo:
                    if (channel == 1) {
                        assert(inputForChannel1);
                        // for legacy stereo mode, dsp1 gets input from right input
                        input = inputForChannel1->getVoltage(0);
                    }
                    assert(numChannels == 2);
                    break;
                case Modes::rightOnly:
                    assert(channel == 0);
                    input = inputForChannel0->getVoltage(0);
                    break;
                case Modes::normal:
                case Modes::leftOnly:
                    break;
                default:
                    assert(false);
            }
            filt.run(input);
            const float output = (float)filt.getOutput();
            audioOutput.setVoltage(output, channel);
            peak.step(output);
        }
        else {
            // It is poly
            switch (mode) {
                case Modes::stereo: {

                    filt.run(input);
                    const float output = (float)filt.getOutput();
                    audioOutput.setVoltage(output, channel);
                    peak.step(output);
                    
                    LadderFilter<T>& filtR = filterR[channel];
                    float inputR = inputForChannel1->getVoltage(channel);
                    filtR.run(inputR);
                    const float outputr = (float)filtR.getOutput();
                    audioOutputR.setVoltage(outputr, channel);
                    }
                    break; 
                case Modes::rightOnly: {
                    float inputR = inputForChannel0->getVoltage(channel);
                    LadderFilter<T>& filtR = filterR[channel];
                    filtR.run(inputR);
                    const float outputr = (float)filtR.getOutput();
                    audioOutputR.setVoltage(outputr, channel);
                    peak.step(outputr); }
                    break; 
                case Modes::leftOnly: {
                    filt.run(input);
                    const float output = (float)filt.getOutput();
                    audioOutput.setVoltage(output, channel);
                    peak.step(output); }
                    break;   
                default:
                    assert(false);
            }
        }
    }
}