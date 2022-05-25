#pragma once

#include "BiquadFilter.h"
#include "BiquadParams.h"
#include "BiquadState.h"
#include "HilbertFilterDesigner.h"
#include "IComposite.h"
#include "LookupTable.h"
#include "SinOscillator.h"

namespace rack {
namespace engine {
struct Module;
}
}  // namespace rack
using Module = ::rack::engine::Module;

template <class TBase>
class BootyDescription : public IComposite {
public:
    Config getParamValue(int i) override;
    int getNumParams() override;
};

/**
 * Complete Frequency Shifter composite
 *
 * If TBase is WidgetComposite, this class is used as the implementation part of the Booty Shifter module.
 * If TBase is TestComposite, this class may stand alone for unit tests.
 */
template <class TBase>
class FrequencyShifter : public TBase {
public:
    FrequencyShifter(Module* module) : TBase(module) {
    }

    FrequencyShifter() : TBase() {
    }

    virtual ~FrequencyShifter() {
    }

    /** Implement IComposite
     */
    static std::shared_ptr<IComposite> getDescription() {
        return std::make_shared<BootyDescription<TBase>>();
    }

    void setSampleRate(float rate) {
        reciprocalSampleRate = 1 / rate;
        HilbertFilterDesigner<T>::design(rate, hilbertFilterParamsSin, hilbertFilterParamsCos);
    }

    // must be called after setSampleRate
    void init() {
        SinOscillator<T, true>::setFrequency(oscParams, T(.01));
        exponential2 = ObjectCache<T>::getExp2();  // Get a shared copy of the 2**x lookup.
                                                   // This will enable exp mode to track at
                                                   // 1V/ octave.
    }

    // Define all the enums here. This will let the tests and the widget access them.
    enum ParamIds {
        PITCH_PARAM,  // the big pitch knob
        NUM_PARAMS
    };

    enum InputIds {
        AUDIO_INPUT,
        CV_INPUT,
        AUDIO_R_INPUT,
        NUM_INPUTS
    };

    enum OutputIds {
        SIN_OUTPUT,
        COS_OUTPUT,
        SIN_R_OUTPUT,
        COS_R_OUTPUT,
        NUM_OUTPUTS
    };

    enum LightIds {
        NUM_LIGHTS
    };

    /**
     * Main processing entry point. Called every sample
     */
    void step() override;

    typedef float T;  // use floats for all signals
    T freqRange = 5;  // the freq range switch
private:
    SinOscillatorParams<T> oscParams;
    SinOscillatorState<T> oscState;
    SinOscillatorState<T> oscStateR;
    BiquadParams<T, 3> hilbertFilterParamsSin;
    BiquadParams<T, 3> hilbertFilterParamsCos;
    BiquadState<T, 3> hilbertFilterStateSin;
    BiquadState<T, 3> hilbertFilterStateCos;
    BiquadState<T, 3> hilbertFilterStateSinR;
    BiquadState<T, 3> hilbertFilterStateCosR;

    std::shared_ptr<LookupTableParams<T>> exponential2;

    float reciprocalSampleRate;
};

template <class TBase>
inline void FrequencyShifter<TBase>::step() {
    assert(exponential2->isValid());

    // Add the knob and the CV value.
    T freqHz;
    T cvTotal = TBase::params[PITCH_PARAM].value + TBase::inputs[CV_INPUT].getVoltage(0);
    if (cvTotal > 5) {
        cvTotal = 5;
    }
    if (cvTotal < -5) {
        cvTotal = -5;
    }
    if (freqRange > .2) {
        cvTotal *= freqRange;
        cvTotal *= T(1. / 5.);
        freqHz = cvTotal;
    } else {
        cvTotal += 7;  // shift up to GE 2 (min value for out 1v/oct lookup)
        freqHz = LookupTable<T>::lookup(*exponential2, cvTotal);
        freqHz /= 2;  // down to 2..2k range that we want.
    }

    SinOscillator<float, true>::setFrequency(oscParams, freqHz * reciprocalSampleRate);

    // Generate the quadrature sin oscillators.
    T x, xR, y, yR;
    SinOscillator<T, true>::runQuadrature(x, y, oscState, oscParams);
    SinOscillator<T, true>::runQuadrature(xR, yR, oscStateR, oscParams);

    // Filter the input through the quadrature filter
    const T input = TBase::inputs[AUDIO_INPUT].getVoltage(0);
    const T inputR = TBase::inputs[AUDIO_R_INPUT].getVoltage(0);
    const T hilbertSin = BiquadFilter<T>::run(input, hilbertFilterStateSin, hilbertFilterParamsSin);
    const T hilbertCos = BiquadFilter<T>::run(input, hilbertFilterStateCos, hilbertFilterParamsCos);
    const T hilbertSinR = BiquadFilter<T>::run(inputR, hilbertFilterStateSinR, hilbertFilterParamsSin);
    const T hilbertCosR = BiquadFilter<T>::run(inputR, hilbertFilterStateCosR, hilbertFilterParamsCos);
    // Cross modulate the two sections.
    x *= hilbertSin;
    y *= hilbertCos;
    xR *= hilbertSinR;
    yR *= hilbertCosR;

    // And combine for final SSB output.
    TBase::outputs[SIN_OUTPUT].setVoltage(x + y, 0);
    TBase::outputs[SIN_R_OUTPUT].setVoltage(xR + yR, 0);
    TBase::outputs[COS_OUTPUT].setVoltage(x - y, 0);
    TBase::outputs[COS_R_OUTPUT].setVoltage(xR - yR, 0);
}

template <class TBase>
int BootyDescription<TBase>::getNumParams() {
    return FrequencyShifter<TBase>::NUM_PARAMS;
}

template <class TBase>
IComposite::Config BootyDescription<TBase>::getParamValue(int i) {
    IComposite::Config ret = {0.0f, 1.0f, 0.0f, "Code type"};
    switch (i) {
        case FrequencyShifter<TBase>::PITCH_PARAM:  // the big pitch knob
            ret = {-5.0, 5.0, 0.0, "Pitch shift"};
            break;
        default:
            assert(false);
    }
    return ret;
}
