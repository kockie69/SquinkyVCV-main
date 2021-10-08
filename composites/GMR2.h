
// Merge conflict - file totally changed.
// This is the one that was on gmr3-clock

#pragma once

#include <memory>

#include "GenerativeTriggerGenerator2.h"
#include "IComposite.h"
#include "ObjectCache.h"
#include "SeqClock2.h"
#include "TimeUtils.h"
#include "TriggerOutput.h"

namespace rack {
namespace engine {
struct Module;
}
}  // namespace rack
using Module = ::rack::engine::Module;

template <class TBase>
class GMR2Description : public IComposite {
public:
    Config getParamValue(int i) override;
    int getNumParams() override;
};

/**
 */
template <class TBase>
class GMR2 : public TBase {
public:
    GMR2(Module* module) : TBase(module), clock(0), runStopProcessor(true) {
    }
    GMR2() : TBase(), clock(0), runStopProcessor(true) {
    }
    void setSampleRate(float rate) {
        reciprocalSampleRate = 1 / rate;
    }

    // must be called after setSampleRate
    void init();
    void setGrammar(StochasticGrammarPtr gmr);

    enum ParamIds {
        CLOCK_INPUT_PARAM,
        RUNNING_PARAM,
        DUMMY_PARAM1,
        DUMMY_PARAM2,
        DUMMY_PARAM3,
        DUMMY_PARAM4,
        DUMMY_PARAM5,
        DUMMY_PARAM6,
        NUM_PARAMS
    };

    enum InputIds {
        CLOCK_INPUT,
        RESET_INPUT,
        RUN_INPUT,
        NUM_INPUTS
    };

    enum OutputIds {
        TRIGGER_OUTPUT,
        NUM_OUTPUTS
    };

    enum LightIds {
        RUN_STOP_LIGHT,  // not currently displayed
        NUM_LIGHTS
    };

    /** Implement IComposite
     */
    static std::shared_ptr<IComposite> getDescription() {
        return std::make_shared<GMR2Description<TBase>>();
    }

    /**
     * Main processing entry point. Called every sample
     */
    void process(const typename TBase::ProcessArgs& args) override;

    bool isRunning() const;
    void reset(bool);
    void allGatesOff();

private:
    float reciprocalSampleRate = 0;
    bool runStopRequested = false;
    std::shared_ptr<GenerativeTriggerGenerator2> gtg;
    // GateTrigger inputClockProcessing;
    SeqClock2 clock;

    TriggerOutput outputProcessing;
    GateTrigger runStopProcessor;

    void serviceRunStop();
};

template <class TBase>
inline void GMR2<TBase>::init() {
}

template <class TBase>
inline void GMR2<TBase>::setGrammar(StochasticGrammarPtr grammar) {
    //SQINFO("GMR2::setGrammar in");
    if (!gtg) {
        gtg = std::make_shared<GenerativeTriggerGenerator2>(
            AudioMath::random(),
            grammar);
    } else {
        gtg->setGrammar(grammar);
    }
    //SQINFO("GMR2::setGrammar out");
}

template <class TBase>
inline void GMR2<TBase>::process(const typename TBase::ProcessArgs& args) {
    assert(gtg);

    serviceRunStop();
    const SeqClock2::ClockRate clockRate = SeqClock2::ClockRate((int)std::round(TBase::params[CLOCK_INPUT_PARAM].value));
    //const float tempo = TBase::params[TEMPO_PARAM].value;
    clock.setup(clockRate, 0, TBase::engineGetSampleTime());

    // and the clock input
    const float extClock = TBase::inputs[CLOCK_INPUT].getVoltage(0);

    // now call the clock
    const float reset = TBase::inputs[RESET_INPUT].getVoltage(0);
    const bool running = isRunning();

    int n = 1;  // todo: run every clock like this?
    int samplesElapsed = n;

    // Our level sensitive reset will get turned into an edge in here
    SeqClock2::ClockResults results = clock.update(samplesElapsed, extClock, running, reset);
    if (results.didReset) {
        this->reset(true);

        // TODO: does this make sense in this module (from SEQ)
        allGatesOff();  // turn everything off on reset, just in case of stuck notes.
    }

    {
        static double last = -1;
        if (results.totalElapsedTime != last) {
            //SQINFO("in GMR2 147, totalElapsed=%f", results.totalElapsedTime);
            last = results.totalElapsedTime;
        }
    }
    bool outClock = gtg->updateToMetricTime(results.totalElapsedTime, float(clock.getMetricTimePerClock()), running);

    outputProcessing.go(outClock);
    TBase::outputs[TRIGGER_OUTPUT].setVoltage(outputProcessing.get(), 0);
}

// does this belong here?
template <class TBase>
bool GMR2<TBase>::isRunning() const {
    bool ret = TBase::params[RUNNING_PARAM].value > .5;
    // assert(ret);
    return ret;
}

template <class TBase>
void GMR2<TBase>::serviceRunStop() {
    runStopProcessor.go(TBase::inputs[RUN_INPUT].getVoltage(0));
    if (runStopProcessor.trigger() || runStopRequested) {
        runStopRequested = false;
        bool curValue = isRunning();
        curValue = !curValue;
        TBase::params[RUNNING_PARAM].value = curValue ? 1.f : 0.f;
    }
    TBase::lights[RUN_STOP_LIGHT].value = TBase::params[RUNNING_PARAM].value;
}

template <class TBase>
void GMR2<TBase>::reset(bool) {
    gtg->queueReset();
}

template <class TBase>
void GMR2<TBase>::allGatesOff() {
    //SQWARN("all gates off does nothing");
}

template <class TBase>
int GMR2Description<TBase>::getNumParams() {
    return GMR2<TBase>::NUM_PARAMS;
}

template <class TBase>
inline IComposite::Config GMR2Description<TBase>::getParamValue(int i) {
    Config ret(0, 1, 0, "");
    switch (i) {
        case GMR2<TBase>::DUMMY_PARAM1:
            ret = {0, 1, 0, "dummy1"};
            break;
        case GMR2<TBase>::DUMMY_PARAM2:
            ret = {0, 1, 0, "dummy2"};
            break;
        case GMR2<TBase>::DUMMY_PARAM3:
            ret = {0, 1, 0, "dummy3"};
            break;
        case GMR2<TBase>::DUMMY_PARAM4:
            ret = {0, 1, 0, "dummy4"};
            break;
        case GMR2<TBase>::DUMMY_PARAM5:
            ret = {0, 1, 0, "dummy5"};
            break;
        case GMR2<TBase>::DUMMY_PARAM6:
            ret = {0, 1, 0, "dummy6"};
            break;
        case GMR2<TBase>::CLOCK_INPUT_PARAM: {
            float low = 0;
            float high = int(SeqClock2::ClockRate::NUM_CLOCKS) + 1;
            float def = int(SeqClock2::ClockRate::Div96);
            ret = {low, high, def, "Clock Rate"};
        } break;
        case GMR2<TBase>::RUNNING_PARAM:
            ret = {0, 1, 1, "running"};
            break;
        default:
            assert(false);
    }
    return ret;
}

#if 0   // this is the version that was in main/gmr3

#pragma once

#include <memory>

#include "GenerativeTriggerGenerator2.h"
#include "IComposite.h"
#include "ObjectCache.h"
#include "TriggerOutput.h"

namespace rack {
namespace engine {
struct Module;
}
}  // namespace rack
using Module = ::rack::engine::Module;

template <class TBase>
class GMR2Description : public IComposite {
public:
    Config getParamValue(int i) override;
    int getNumParams() override;
};

/**
 */
template <class TBase>
class GMR2 : public TBase {
public:
    GMR2(Module* module) : TBase(module), inputClockProcessing(true) {
    }
    GMR2() : TBase(), inputClockProcessing(true) {
    }
    ~GMR2() { //SQINFO("dtor of GMR2 composite");
     }
    void setSampleRate(float rate) {
        reciprocalSampleRate = 1 / rate;
    }

    // must be called after setSampleRate
    void init();
    void setGrammar(StochasticGrammarPtr gmr);

    enum ParamIds {
        DUMMY_PARAM1,
        DUMMY_PARAM2,
        DUMMY_PARAM3,
        DUMMY_PARAM4,
        DUMMY_PARAM5,
        DUMMY_PARAM6,
        NUM_PARAMS
    };

    enum InputIds {
        CLOCK_INPUT,
        NUM_INPUTS
    };

    enum OutputIds {
        TRIGGER_OUTPUT,
        NUM_OUTPUTS
    };

    enum LightIds {
        NUM_LIGHTS
    };

    /** Implement IComposite
     */
    static std::shared_ptr<IComposite> getDescription() {
        return std::make_shared<GMR2Description<TBase>>();
    }

    /**
     * Main processing entry point. Called every sample
     */
    void process(const typename TBase::ProcessArgs& args) override;

private:
    float reciprocalSampleRate = 0;
    std::shared_ptr<GenerativeTriggerGenerator2> gtg;
    GateTrigger inputClockProcessing;
    TriggerOutput outputProcessing;
};

template <class TBase>
inline void GMR2<TBase>::init() {
}

template <class TBase>
inline void GMR2<TBase>::setGrammar(StochasticGrammarPtr grammar) {
    //SQINFO("GMR2::setGrammar in");
    if (!gtg) {
        gtg = std::make_shared<GenerativeTriggerGenerator2>(
        AudioMath::random_better(),
        grammar);
    } else {
        gtg->setGrammar(grammar);
    }
      //SQINFO("GMR2::setGrammar out");
}

template <class TBase>
inline void GMR2<TBase>::process(const typename TBase::ProcessArgs& args) {
  //SQINFO("GMR2<TBase>::process gtg = %p", gtg.get());
    bool outClock = false;
    float inClock = TBase::inputs[CLOCK_INPUT].getVoltage(0);
    inputClockProcessing.go(inClock);
    if (inputClockProcessing.trigger()) {
        assert(gtg);
        outClock = gtg->clock();
    }
    outputProcessing.go(outClock);
    TBase::outputs[TRIGGER_OUTPUT].setVoltage(outputProcessing.get(), 0);
}

template <class TBase>
int GMR2Description<TBase>::getNumParams() {
    return GMR2<TBase>::NUM_PARAMS;
}

template <class TBase>
inline IComposite::Config GMR2Description<TBase>::getParamValue(int i) {
    Config ret(0, 1, 0, "");
    switch (i) {
        case GMR2<TBase>::DUMMY_PARAM1:
            ret = {0, 1, 0, "dummy1"};
            break;
        case GMR2<TBase>::DUMMY_PARAM2:
            ret = {0, 1, 0, "dummy2"};
            break;
        case GMR2<TBase>::DUMMY_PARAM3:
            ret = {0, 1, 0, "dummy3"};
            break;
        case GMR2<TBase>::DUMMY_PARAM4:
            ret = {0, 1, 0, "dummy4"};
            break;
        case GMR2<TBase>::DUMMY_PARAM5:
            ret = {0, 1, 0, "dummy5"};
            break;
        case GMR2<TBase>::DUMMY_PARAM6:
            ret = {0, 1, 0, "dummy6"};
            break;
        default:
            assert(false);
    }
    return ret;
}
#endif