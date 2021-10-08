#pragma once

#include <assert.h>

#include "SqLog.h"
#include "StochasticNote.h"
#include "TimeUtils.h"

class TriggerSequencer {
public:
    enum EVT {
        TRIGGER,
        END
    };
    class Event {
    public:
        short int evt;

        /**
         * delayPPQ is in ppq units (old school)  
         * StochasticNote::ppq is where that lives
         * 
         */
        short int delayPPQ;
    };

    // data is a buffer that will be bound to the sequencer, although it is "owned"
    // by the caller
    TriggerSequencer(const Event* data) : curEvent(data) {
        resetData(data);
    }

    /**
     * Main "play something" function.
     * @param metricTime is the current time where 1 = quarter note.
     * @param quantizationInterval is the amount of metric time in a clock. 
     * So, if the click is a sixteenth note clock, quantizationInterval will be .25
     * 
     * After calling, clients may call getTriggerAndReset or getNeedsDataAndReset
     * to find out what happened.
     */
    void updateToMetricTime(double metricTime, float quantizationInterval, bool running);
    void updateToMetricTimeInternal(double, float);

    void playOnce(double metricTime, float quantizeInterval);

    // reset by concatenating new data to seq
    // note that reset may generate a trigger
    void resetData(const Event* data) {
        //_dump(data, "start of reset, passed in data");

        // reset should always pass in something
        assert(data);
        assert(data->evt != END);
        assert(isValid(data));

        curEvent = data;
        auto newDelayPPQ = data->delayPPQ;
        //SQINFO("TSEQ reset 58: initial delay = %d\n", newDelayPPQ);
        double newDelayMetric = double(newDelayPPQ) / double(StochasticNote::ppq);

        nextEventTimeMetric += newDelayMetric;

        //SQWARN("TriggerSeq::trying to reset");
        assert(curEvent);
        processClocks(0);
        //_dump(curEvent, "in reset, after process clocks");
    }
    void resetToCurrentData() {
        assert(curEvent);
        if (curEvent) {
            // nextEventTimeMetric = curEvent->delayPPQ;

            auto newDelayPPQ = curEvent->delayPPQ;

            double newDelayMetric = double(newDelayPPQ) / double(StochasticNote::ppq);
            nextEventTimeMetric = newDelayMetric;
            //if (nextEventTimeMetric > 0) {
                //SQINFO("data not at zero, %f", nextEventTimeMetric);
            //}
        }
    }

    bool getTriggerAndReset() {
        bool ret = trigger;
        trigger = false;
        return ret;
    }

    // do we need this? Is it the same as getEnd()????
    bool getNeedDataAndReset() {
        bool ret = needData;
        needData = false;
        return ret;
    }

    bool getEnd() const {
        return curEvent == 0;
    }  // did sequencer end after last clock?

    // checks that a sequence is valid
    static bool isValid(const Event* data);
    const Event* _getEvt() const {
        return curEvent;
    }

    static void _dump(const Event* p, const char* msg) {
        fprintf(stderr, "TSEQ::Dump(%s)\n", msg);
        if (!p) {
            fprintf(stderr, "null data pointer\n");
        }
        while (p) {
            fprintf(stderr, "evt=%s del=%d\n",
                    (p->evt == END) ? "END" : "TRIG",
                    p->delayPPQ);
            if (p->evt == END) {
                p = nullptr;
            } else {
                ++p;
            }
        }
        fflush(stderr);
    }

private:
    // This is a pointer that is marching up and array of events that were set
    // by ::reset();
    const Event* curEvent;
    bool trigger = false;
    bool needData = false;

    // we used to be delay based, but with new clock let's be abs time based
    //double delayMetric = 0;
    double nextEventTimeMetric = 0;

    void processClocks(double metricTime);
};

inline void TriggerSequencer::playOnce(double metricTime, float quantizeInterval) {
    //SQINFO("TSeq::play once, metric %f", metricTime);

    // const double delayMetricQuantized = TimeUtils::quantize(delayMetric, quantizeInterval, true);
    const double eventTimeMetricQuantized = TimeUtils::quantize(nextEventTimeMetric, quantizeInterval, true);
    if (eventTimeMetricQuantized <= metricTime) {
        // here we do something
        // I think there is a mismatch here -
        // metric time is 1 = quarter note.
        // but delay is ppq = quarter note.
        // dp we need to accumulate error here?
        // I think delay needs to be in metric time (double).
        // we already set this->trigger, so I don't think we should change the meaning of our return value.
        // it just means we did something

        //SQINFO("*** play once now nextEventTimeMetric = %f", nextEventTimeMetric);

        // processClocks will update trigger, etc.
        //     delayMetric -= delayMetricQuantized;
        //SQINFO("*** play once now delay metric after = %f", delayMetric);
        processClocks(metricTime);
    }
}

inline void TriggerSequencer::updateToMetricTime(double metricTime, float quantizationInterval, bool running) {
    //SQWARN("update to metric time is fake");
    assert(quantizationInterval != 0);

    if (!running) {
        // If seq is paused, leave now so we don't act on the dirty flag when stopped.
        return;
    }

    updateToMetricTimeInternal(metricTime, quantizationInterval);
}

inline void TriggerSequencer::updateToMetricTimeInternal(double metricTime, float quantizationInterval) {
    metricTime = TimeUtils::quantize(metricTime, quantizationInterval, true);
    // If we had a conflict and needed to reset, then
    // start all over from beginning. Or, if reset initiated by user.

    const bool isReset = false;  // TODO: this was a member variable before. probably need it to imp reset
    if (isReset) {
        // printf("\nupdatetometrictimeinternal  player proc reset\n");
        assert(false);
        /*
        curEvent = track->begin();
        resetAllVoices(isResetGates);
        voiceAssigner.reset();
        isReset = false;
        isResetGates = false;
        currentLoopIterationStart = 0;
        */
    }

    // To implement loop start, we just push metric time up to where we want to start.
    // TODO: skip over initial stuff?

#if 0
    if (song->getSubrangeLoop().enabled) {
        // if (loopParams && loopParams.load()->enabled) {
        metricTime += song->getSubrangeLoop().startTime;
    }
#endif

    playOnce(metricTime, quantizationInterval);
}

inline void TriggerSequencer::processClocks(double curMetricTime) {
    trigger = false;
    //printf("enter proc clock, curevt =%p, delay = %d\n", curEvent, delay);
    if (!curEvent) {
        //SQINFO("--- leave clock early - ended\n");
        return;  // seq is stopped
    }

    while (nextEventTimeMetric <= curMetricTime) {
        //SQINFO("delay went to %f, evt=%d", delayMetric, curEvent->evt);
        //SQINFO("in TSEQ::process clock loop, evt=%f, cur=%f", nextEventTimeMetric, curMetricTime);
        //SQINFO("in proc clock, firing %f <= %f evt=%d", nextEventTimeMetric, curMetricTime, curEvent->evt);
        switch (curEvent->evt) {
            case END:
                //SQINFO("process clock setting end at 188\n");
                curEvent = 0;  // stop seq by clering ptr
                needData = true;
                return;
            case TRIGGER:
                trigger = true;
                ++curEvent;  // and go to next one
                //SQINFO("trigger set true");
                break;

            default:
                assert(false);
        }

        auto newDelayPPQ = curEvent->delayPPQ;
        //SQINFO("Process Clocks fetching new delay ppq = %d", newDelayPPQ);
        double newDelayMetric = double(newDelayPPQ) / double(StochasticNote::ppq);

        nextEventTimeMetric += newDelayMetric;
    }
};

inline bool TriggerSequencer::isValid(const Event* data) {
    while (data->evt != END) {
        assert(data->evt == TRIGGER);
        assert(data->delayPPQ >= 0);
        assert(data->delayPPQ < 2000);  // just for now - we expect them to be small
        ++data;
    }

    return true;
}

