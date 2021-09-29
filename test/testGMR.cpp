
#include "asserts.h"

#include "GMR2.h"
#include "TestComposite.h"
#include "tutil.h"

#include <set>

using G2 = GMR2<TestComposite>;


// test that we get triggers out
static void test0()
{
    //SQINFO("\n\n\n-------------- testGMR-0 -------------");
    G2 gmr;
    std::set<float> data;

    initComposite(gmr);
    gmr.setSampleRate(44100);
    gmr.init();
    gmr.setGrammar(StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::demo));

    TestComposite::ProcessArgs args;
    for (int i = 0; i < 10; ++i) {
        //SQINFO("---- clock low");
        gmr.inputs[G2::CLOCK_INPUT].setVoltage(0, 0);
        for (int i = 0; i < 100; ++i) {
            gmr.process(args);
            float out = gmr.outputs[G2::TRIGGER_OUTPUT].getVoltage(0);
            data.insert(out);
        }

        //SQINFO("---clock high");
        gmr.inputs[G2::CLOCK_INPUT].setVoltage(10, 0);
        for (int i = 0; i < 100; ++i) {
            gmr.process(args);
            float out = gmr.outputs[G2::TRIGGER_OUTPUT].getVoltage(0);
            data.insert(out);
        }
    }

    assert(data.find(cGateOutHi) != data.end());
    assert(data.find(cGateOutLow) != data.end());
    assertEQ(data.size(), 2);
  //  assert(false);
}


static void test1()
{
    //SQINFO("\n\n\n-------------- testGMR2-1 -------------");
    G2 gmr;
    initComposite(gmr);
   // std::set<float> data;
  //  std::map<
    int transitions = 0;

    gmr.setSampleRate(44100);
    gmr.init();
    gmr.setGrammar(StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::demo));

    TestComposite::ProcessArgs args;
    float lastOut = 0;
    for (int i = 0; i < 1000; ++i) {

        gmr.inputs[G2::CLOCK_INPUT].setVoltage(0, 0);
        for (int i = 0; i < 100; ++i) {
            gmr.process(args);

            float out = gmr.outputs[G2::TRIGGER_OUTPUT].getVoltage(0);
            if (out != lastOut) {
                transitions++;
                lastOut = out;
            }
          
        }
        gmr.inputs[G2::CLOCK_INPUT].setVoltage(10, 0);
        for (int i = 0; i < 100; ++i) {
            gmr.process(args);
            float out = gmr.outputs[G2::TRIGGER_OUTPUT].getVoltage(0);
 
            if (out != lastOut) {
                transitions++;
                lastOut = out;
            }
        }
    }

    //SQINFO("transitions = %d", transitions);
    assertGT(transitions, 20);
}


void testGMR()
{
    test0();
    test1();
}
