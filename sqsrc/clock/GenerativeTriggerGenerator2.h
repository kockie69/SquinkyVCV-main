#pragma once

#include "AudioMath.h"
#include "StochasticGrammar2.h"
#include "StochasticProductionRule.h"
#include "TriggerSequencer.h"

/* Knows how to generate trigger sequence data
 * when evaluating a grammar
 */

class GTGEvaluator2 : public StochasticProductionRule::EvaluationState {
public:
    GTGEvaluator2(AudioMath::RandomUniformFunc xr, TriggerSequencer::Event* buf) : StochasticProductionRule::EvaluationState(xr),
                                                                                   _buf(buf) {
    }

    // void writeSymbol(int fakeGKey) override
    void writeSymbol(const StochasticNote& note) override {
        // first: write out a trigger at "current delay"
        _buf->evt = TriggerSequencer::TRIGGER;
        _buf->delayPPQ = _delayPPQ;
        ++_buf;

        // then set current delay to duration of key
        _delayPPQ = note.duration;
        //SQINFO("will delay %d. is that correct?", _delay);
    }

    // call this to write final event
    void writeEnd() {
        _buf->evt = TriggerSequencer::END;
        _buf->delayPPQ = _delayPPQ;
    }

private:
    TriggerSequencer::Event* _buf;
    int _delayPPQ = 0;
};

/* wraps up some stochastic gnerative grammar stuff feeding
 * a trigger sequencer
 */
class GenerativeTriggerGenerator2 {
public:
    GenerativeTriggerGenerator2(AudioMath::RandomUniformFunc r, StochasticGrammarPtr grammar) : _r(r),
                                                                                                _grammar(grammar) {
        generate();
        _seq = new TriggerSequencer(_data);
    }

    ~GenerativeTriggerGenerator2() {
        delete _seq;
    }

    void setGrammar(StochasticGrammarPtr grammar) {
        _grammar = grammar;
    }

    void queueReset() {
        resetRequested = true;
    }

    /**
     * Main "play something" function.
     * @param metricTime is the current time where 1 = quarter note.
     * @param quantizationInterval is the amount of metric time in a clock. 
     * So, if the click is a sixteenth note clock, quantizationInterval will be .25
     */
    bool updateToMetricTime(double metricTime, float quantizationInterval, bool running);

private:
    TriggerSequencer* _seq = nullptr;
    TriggerSequencer::Event _data[33];
    AudioMath::RandomUniformFunc _r;
    StochasticGrammarPtr _grammar;
    bool resetRequested = false;
    void generate();
};

inline bool GenerativeTriggerGenerator2::updateToMetricTime(double metricTime, float quantizationInterval, bool running) {
    // TODO: args are from old seq - not appropriate for us
    //SQWARN("GTG updateToMetricTime(%f", metricTime);
    // assert(!resetRequested);
    if (resetRequested) {
        resetRequested = false;
        static int count = 0;
        ++count;
        generate();
        _seq->resetToCurrentData();
    }

    _seq->updateToMetricTime(metricTime, quantizationInterval, running);
    const bool needData = _seq->getNeedDataAndReset();

    if (needData) {
        //SQINFO("GTG::calling generator from updateToMetricTime (%f)", metricTime);
        generate();
        _seq->updateToMetricTime(metricTime, quantizationInterval, running);
        assert(!_seq->getNeedDataAndReset());
    }
    return _seq->getTriggerAndReset();
}

inline void GenerativeTriggerGenerator2::generate() {
    //SQINFO("---------- gtg generate new --------------");
    assert(_grammar);
    GTGEvaluator2 es(_r, _data);
    es.grammar = _grammar;
    auto baseRule = es.grammar->getRootRule();
    assert(baseRule);
    StochasticProductionRule::evaluate(es, baseRule);
    es.writeEnd();
    TriggerSequencer::isValid(_data);

    // during construction this gets called before _seq exists
    if (_seq) {
        _seq->resetData(_data);
        assert(!_seq->getEnd());
    }
    //SQINFO("---------- generate new exit --------------");
}