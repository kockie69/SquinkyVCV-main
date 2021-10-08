
#include <set>

#include "GMR2.h"
#include "Seq.h"
#include "TestComposite.h"
#include "asserts.h"
#include "tutil.h"

using G = GMR2<TestComposite>;

// test that we get triggers out
static void test0() {
    //SQINFO("-------------- testGMR2-0 -------------");
    G gmr;
    initComposite(gmr);
    std::set<float> data;

    gmr.setSampleRate(44100);
    gmr.init();

    StochasticGrammarPtr grammar = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::quarters);
    gmr.setGrammar(grammar);
    TestComposite::ProcessArgs args;

    for (int i = 0; i < 10; ++i) {
        gmr.inputs[G::CLOCK_INPUT].setVoltage(0, 0);
        for (int i = 0; i < 100; ++i) {
            gmr.process(args);

            float out = gmr.outputs[G::TRIGGER_OUTPUT].getVoltage(0);
            //SQINFO("in test add %f", out);
            data.insert(out);
        }
        gmr.inputs[G::CLOCK_INPUT].setVoltage(10, 0);
        for (int i = 0; i < 100; ++i) {
            gmr.process(args);
            float out = gmr.outputs[G::TRIGGER_OUTPUT].getVoltage(0);
            //SQINFO("in test add %f", out);
            data.insert(out);
        }
    }

    // we should see a high and a log
    assert(data.find(cGateOutHi) != data.end());
    assert(data.find(cGateOutLow) != data.end());
    assertEQ(data.size(), 2);
}

static void test1() {
    //SQINFO("-------------- testGMR2-1 -------------");
    G gmr;
    initComposite(gmr);
    // std::set<float> data;
    //  std::map<
    int transitions = 0;

    gmr.setSampleRate(44100);
    gmr.init();
    StochasticGrammarPtr grammar = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::quarters);
    gmr.setGrammar(grammar);

    TestComposite::ProcessArgs args;

    float lastOut = 0;
    for (int i = 0; i < 1000; ++i) {
        gmr.inputs[G::CLOCK_INPUT].setVoltage(0, 0);
        for (int i = 0; i < 100; ++i) {
            gmr.process(args);

            float out = gmr.outputs[G::TRIGGER_OUTPUT].getVoltage(0);
            if (out != lastOut) {
                transitions++;
                lastOut = out;
            }
        }
        gmr.inputs[G::CLOCK_INPUT].setVoltage(10, 0);
        for (int i = 0; i < 100; ++i) {
            gmr.process(args);
            float out = gmr.outputs[G::TRIGGER_OUTPUT].getVoltage(0);

            if (out != lastOut) {
                transitions++;
                lastOut = out;
            }
        }
    }

    assertGT(transitions, 20);
}

static void testQuarters() {
    //SQINFO("-------------- testGMR2-quarter -------------");
    G gmr;
    initComposite(gmr);
    // std::set<float> data;
    //  std::map<
    int transitions = 0;

    gmr.setSampleRate(44100);
    gmr.init();
    TestComposite::ProcessArgs args;

    gmr.setGrammar(StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::quarters));

    float lastOut = 0;
    int pos = 0;
    int lastCross = 0;

    // elapsed is how many input clocks have gone by since the last output transition.
    int elapsedClocks = 0;

    // let's feed a clocks of a fast rate
    for (int i = 0; i < 1000; ++i) {
        gmr.inputs[G::CLOCK_INPUT].setVoltage(0, 0);
        for (int i = 0; i < 10; ++i) {
            gmr.process(args);

            float out = gmr.outputs[G::TRIGGER_OUTPUT].getVoltage(0);
            if (out != lastOut) {
                transitions++;
                lastOut = out;
                //SQINFO("pos = %d out=%f last = %d elapsed1=%d", pos, out, lastCross, elapsedClocks);
                lastCross = 0;
                elapsedClocks = 0;
            }
            ++pos;
            ++lastCross;
        }

        gmr.inputs[G::CLOCK_INPUT].setVoltage(10, 0);
        for (int i = 0; i < 10; ++i) {
            gmr.process(args);
            float out = gmr.outputs[G::TRIGGER_OUTPUT].getVoltage(0);

            if (out != lastOut) {
                transitions++;
                lastOut = out;
                //SQINFO("pos = %d out=%f last = %d elapsed2=%d", pos, out, lastCross, elapsedClocks);
                lastCross = 0;
                elapsedClocks = 0;
            }
            ++pos;
            ++lastCross;
        }
        elapsedClocks++;
    }

    // Let's assert that the result is right
    //SQWARN("**** let'smake testQuarters work");

    // assert(false);
}

void waitForTriggerToGoAway(G& comp) {
    TestComposite::ProcessArgs args;
    int iter = 0;
    while (true) {
        bool trig = comp.outputs[G::TRIGGER_OUTPUT].value > 5;
        if (!trig) {
            //SQINFO("waitForTriggerToGoAway waited %d", iter);
            return;
        }
        comp.process(args);
        ++iter;

        assert(iter < (TRIGGER_OUT_TIME_MS * 44100 / 1000));
        assert(iter < 40000);
    }
}

int clockAndCount(G& comp, int clockCycles) {
    int ret = 0;
    TestComposite::ProcessArgs args;
    bool oldOut = false;

    // Will get weird results it this is called with a trigger
    assert(comp.outputs[G::TRIGGER_OUTPUT].value < 5);

    while (clockCycles--) {
        comp.inputs[G::CLOCK_INPUT].value = 10;
        for (int i = 0; i < 10; ++i) {
            comp.process(args);
            const bool out = comp.outputs[G::TRIGGER_OUTPUT].value > 5;
            if (out && !oldOut) {
                ret++;
            }
            oldOut = out;
        }
        comp.inputs[G::CLOCK_INPUT].value = 0;
        for (int i = 0; i < 10; ++i) {
            comp.process(args);
            const bool out = comp.outputs[G::TRIGGER_OUTPUT].value > 5;
            if (out && !oldOut) {
                ret++;
            }
            oldOut = out;
        }
    }
    return ret;
}

static void testRunStop() {
    G gmr;
    initComposite(gmr);
    gmr.inputs[G::RUN_INPUT].value = 0;
    int transitions = 0;

    gmr.setSampleRate(44100);
    gmr.init();
    gmr.params[G::CLOCK_INPUT_PARAM].value = float(SeqClock2::ClockRate::Div64);
    TestComposite::ProcessArgs args;
    gmr.setGrammar(StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::quarters));
    assert(gmr.isRunning());

    // clock it running
    int x = clockAndCount(gmr, 200);
    assertGT(x, 3);

    // low to hi transition will stop it
    // wait for a stretched trigger to go away.
    gmr.inputs[G::RUN_INPUT].value = 10;
    waitForTriggerToGoAway(gmr);
    x = clockAndCount(gmr, 200);
    assert(!gmr.isRunning());
    assertEQ(x, 0);

    // hi to low, stays off
    gmr.inputs[G::RUN_INPUT].value = 10;
    x = clockAndCount(gmr, 200);
    assert(!gmr.isRunning());
    assertEQ(x, 0);
}


/* ok, it think gmr.reset should call gtg.reset
 gtg reset should set isReset
 gtg.update to metric times should act on isReset and clear it.

 (above is like set)
 */
static void testReset() {
    TestComposite::ProcessArgs args;
    G gmr;

    initComposite(gmr);
    gmr.inputs[G::RUN_INPUT].value = 0;
    int transitions = 0;

    gmr.setSampleRate(44100);
    gmr.init();
    gmr.setGrammar(StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::quarters));
    assert(gmr.isRunning());

    // should have a clock immediately.
    int x = clockAndCount(gmr, 1);
    assertEQ(x, 1);

    // now a small number of clocks clock should not trigger
    waitForTriggerToGoAway(gmr);
    x = clockAndCount(gmr, 10);
    assertEQ(x, 0);

    gmr.inputs[G::RESET_INPUT].value = 10;
    gmr.process(args);

    //SQINFO("back in test");
    bool out = gmr.outputs[G::TRIGGER_OUTPUT].value > 5;
    assert(out);
#if 0
    //SQINFO("first trig out after reset = %d", out);
   // assert(out);
    out = gmr.outputs[G::TRIGGER_OUTPUT].value > 5;
    //SQINFO("second trig out after reset = %d", out);


    assert(false);  // finish me
#endif
}

void testGMR2() {
    test0();
    test1();
    testQuarters();
    testRunStop();
    testReset();
}