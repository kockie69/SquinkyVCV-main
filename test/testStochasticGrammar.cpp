
#include <set>
#include <string>
#include <vector>

#include "GenerativeTriggerGenerator2.h"
#include "TriggerSequencer.h"

const float sixtyFourth = float(1.0 / 64.0);
const float quarter = 1.f;
short int ppq = StochasticNote::ppq;

// TODO: rename this file

/*********************************************************************************************************
 * TriggerSequencer
 **********************************************************************************************************/

// test event at zero fires at zero
// let's try going down the road of assuming that events at
// t == 0 fire in constructor (sort of makes sense)
static void ts0() {
    //SQINFO("------ ts0");
    //  const int ppq = StochasticNote::ppq;
    const short int duration = int(1.5f * float(ppq));
    TriggerSequencer::Event seq[] =
        {
            // trigger at 0, end at 1.5 quarter note
            {TriggerSequencer::TRIGGER, 0},
            {TriggerSequencer::END, duration}};
    TriggerSequencer ts(seq);

    // everything should happen in ctor,
    // it it will come to life triggered and over
    assert(ts.getTriggerAndReset());
    assert(!ts.getTriggerAndReset());

    // actually, we maybe don't care what  state we are in?
    //assert(!ts._getEvt());
}

// test trigger at q happens at q
static void tsQuarter() {
    TriggerSequencer::Event seq[] =
        {
            {TriggerSequencer::TRIGGER, ppq},
            {TriggerSequencer::END, 0}};

    //  const float sixtyFourth = float(1.0 / 64.0);
    TriggerSequencer ts(seq);
    assert(!ts.getTriggerAndReset());

    ts.updateToMetricTime(.5, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());

    ts.updateToMetricTime(.9, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());

    ts.updateToMetricTime(1.0, sixtyFourth, true);
    assert(ts.getTriggerAndReset());

    ts.updateToMetricTime(1.1, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());

    assert(!ts._getEvt());
}

static void tsQuarterMulti() {
    TriggerSequencer::Event seq[] =
        {
            {TriggerSequencer::TRIGGER, ppq},      // 1 q
            {TriggerSequencer::TRIGGER, ppq},      // 2 q
            {TriggerSequencer::TRIGGER, 2 * ppq},  // 4 q
            {TriggerSequencer::TRIGGER, ppq},
            {TriggerSequencer::END, 0}};

    TriggerSequencer ts(seq);
    assert(!ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());

    // almost there
    ts.updateToMetricTime(quarter - sixtyFourth, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());

    // finally there (first delay expired
    ts.updateToMetricTime(quarter, sixtyFourth, true);
    assert(ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());

    // almost there
    ts.updateToMetricTime(2 * quarter - sixtyFourth, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());

    // finally there second delay expired
    ts.updateToMetricTime(2 * quarter, sixtyFourth, true);
    assert(ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());

    // now half note
    // not even close
    ts.updateToMetricTime(3 * quarter - sixtyFourth, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());
    ts.updateToMetricTime(3 * quarter, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());
    ts.updateToMetricTime(3 * quarter + sixtyFourth, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());

    // almost there
    ts.updateToMetricTime(4 * quarter - sixtyFourth, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());

    // finally there third delay expired
    ts.updateToMetricTime(4 * quarter, sixtyFourth, true);
    assert(ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());
}

// I don't know if this "feature" ever gets used in gtg,
// but might as well test it
static void tsTriggerAtEnd() {
    TriggerSequencer::Event seq[] =
        {
            {TriggerSequencer::TRIGGER, ppq},
            {TriggerSequencer::END, 0}};

    TriggerSequencer ts(seq);

    // almost there
    ts.updateToMetricTime(quarter - sixtyFourth, sixtyFourth, true);
    assert(!ts.getTriggerAndReset());
    assert(!ts.getNeedDataAndReset());

    // finally there (first delay expired
    ts.updateToMetricTime(quarter, sixtyFourth, true);
    assert(ts.getTriggerAndReset());
    assert(ts.getNeedDataAndReset());
}

// 4 quarter loop: delay 4, trigger, end
static void tsQuarterLoop() {
    //SQINFO("------- ts2");

    TriggerSequencer::Event seq[] =
        {
            {TriggerSequencer::TRIGGER, ppq},
            {TriggerSequencer::END, 0}};

    TriggerSequencer ts(seq);
    bool firstTime = true;
    // first time through, 4 clocks of nothing. then clock, 0,0,0
    for (int i = 0; i < 4; ++i) {

        float baseTime = i * quarter;
        //ts.clock();

        // just advance a tinny bit into the first quarter
        ts.updateToMetricTime(sixtyFourth, sixtyFourth, true);
        //if (firstTime) {
            assert(!ts.getTriggerAndReset());
            assert(!ts.getEnd());
            assert(!ts.getNeedDataAndReset());
#if 0
            firstTime = false;
        } else {
            //printf("second time around, t=%d e=%d\n", ts.getTrigger(), ts.getEnd());
            // second time around we finally see the trigger
            assert(ts.getTriggerAndReset());

            // second time around, need to clock the end of the last time
            assert(ts.getEnd());
            assert(ts.getNeedDataAndReset());
            ts.reset(seq);                     // start it up again
            assert(!ts.getTriggerAndReset());  // resetting should not set us up for a trigger
        }
#endif


        ts.updateToMetricTime(baseTime + (2 * sixtyFourth), sixtyFourth, true);
        assert(!ts.getTriggerAndReset());
        assert(!ts.getEnd());
        assert(!ts.getNeedDataAndReset());

        // almost there
        ts.updateToMetricTime(baseTime + (quarter - sixtyFourth), sixtyFourth, true);
        assert(!ts.getTriggerAndReset());
        assert(!ts.getEnd());
        assert(!ts.getNeedDataAndReset());

        // finally there
        ts.updateToMetricTime(baseTime + (quarter), sixtyFourth, true);
        assert(ts.getTriggerAndReset());
        assert(ts.getNeedDataAndReset());
        assert(ts.getEnd());

        // we played it all, so start on next loop
        ts.resetData(seq);
       // assert(false);

#if 0
        ts.clock();
        assert(!ts.getTrigger());
        assert(!ts.getEnd());

        ts.clock();
        assert(!ts.getTrigger());
#endif
        //	assert(ts.getEnd());

        //	ts.reset(seq);
    }
}

#if 0
// test trigger seq qith
// 4 clock loop: trigger, delay 4 end
static void ts3() {
    TriggerSequencer::Event seq[] =
        {
            {TriggerSequencer::TRIGGER, 0},
            {TriggerSequencer::END, 4}};
    TriggerSequencer ts(seq);

    bool firstLoop = true;
    for (int i = 0; i < 4; ++i) {
        //printf("--- loop ----\n");

        // 1

        ts.clock();
        if (firstLoop) {
            assert(ts.getTrigger());
            // we just primed loop at top, so it's got a ways
            assert(!ts.getEnd());
            firstLoop = false;
        } else {
            // second time around, need to clock the end of the last time
            assert(ts.getEnd());
            ts.reset(seq);            // start it up again
            assert(ts.getTrigger());  // resetting should have set us up for a trigger
        }
        // 2
        ts.clock();
        assert(!ts.getTrigger());
        assert(!ts.getEnd());
        // 3
        ts.clock();
        assert(!ts.getTrigger());
        assert(!ts.getEnd());
        // 4
        ts.clock();
        assert(!ts.getTrigger());
        assert(!ts.getEnd());
    }
}

// test trigger seq with straight ahead 4/4 as generated by a grammar
static void ts4() {
    TriggerSequencer::Event seq[] =
        {
            {TriggerSequencer::TRIGGER, 0},
            {TriggerSequencer::TRIGGER, 4},
            {TriggerSequencer::TRIGGER, 4},
            {TriggerSequencer::TRIGGER, 4},
            {TriggerSequencer::END, 4}};
    TriggerSequencer ts(seq);

    //bool firstLoop = true;
    for (int i = 0; i < 100; ++i) {
        bool firstTime = (i == 0);
        // repeating pattern of trigg, no, no, no
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                //	printf("test loop, i=%d, j=%d, k=%d\n", i, j, k);
                ts.clock();

                bool expectEnd = (k == 0) && (j == 0) && !firstTime;
                assert(ts.getEnd() == expectEnd);
                if (ts.getEnd()) {
                    ts.reset(seq);
                }
                assert(ts.getTrigger() == (k == 0));
            }
        }
    }
}
#endif

/********************************************************************************************
* GenerativeTriggerGenerator
**********************************************************************************************/

static void gtgFirstNote() {
    //SQINFO("\n\n****************************** gtgFirstNote ");
    //  GKEY key = init1();

    StochasticGrammarPtr grammar = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::quarters);

    GenerativeTriggerGenerator2 gtg(AudioMath::random(), grammar);
    bool yes = false;
    bool no = false;

    // clock into the first note
    bool ck = gtg.updateToMetricTime(.25, sixtyFourth, true);
    assert(ck);
}

// test that we get some clocks and some not
#if 1
static void gtg0() {
    //SQINFO("\n\n****************************** gtg0 ");
    //  GKEY key = init1();
    const float sixtyFourth = float(1.0 / 64.0);
    StochasticGrammarPtr grammar = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::quarters);

    GenerativeTriggerGenerator2 gtg(AudioMath::random(), grammar);
    bool yes = false;
    bool no = false;
    for (int i = 0; i < 100; ++i) {
        //SQINFO("In loop of gtg0 test, i=%d", i);
        double time = i * .5;  // clock at eighth notes
        bool ck = gtg.updateToMetricTime(time, sixtyFourth, true);
        if (ck)
            yes = true;
        else
            no = true;
#if 0
        if (gtg.clock())
            yes = true;
        else
            no = true;

        if (yes && no) {
            //printf("clocked at %d\n", i);
            return;
        }
#endif
    }
    assert(yes);
    assert(no);

    //SQWARN("finish me");
    // assert(false);
}
#endif

#if 0

// test that we get everything in even quarter notes
static void gtg1() {
    GKEY key = init1();
    std::set<int> counts;

    GenerativeTriggerGenerator gtg(AudioMath::random(), rules, numRules, key);

    int ct = 0;
    for (int i = 0; i < 10000; ++i) {
        bool b = gtg.clock();
        if (b) {
            //printf("clocked at %d\n", ct);
            counts.insert(ct);
            ct = 0;
        }
        ct++;
    }
    //counts.insert(50);
    assert(!counts.empty());
    for (std::set<int>::iterator it = counts.begin(); it != counts.end(); ++it) {
        int c = *it;

        if ((c % PPQ) != 0) {
            //printf("PPQ=%d, c modePPQ =%d\n", PPQ, (c % PPQ));
            //printf("2ppq = %d, 4ppq=%d\n", 2 * PPQ, 4 * PPQ);
            assert(false);
        }
    }
}
#endif

void testStochasticGrammar() {
    ts0();
    tsQuarter();
    tsQuarterMulti();
    tsTriggerAtEnd();
    tsQuarterLoop();

    //  assert(false);          // make these work
    gtgFirstNote();
    gtg0();  // try first to debug

#if 0
    ts2();
    ts3();
    ts4();
#endif

#ifdef _DEBUG
    // gdt1();
#endif

    // assert(false);  // put everythign back
    //SQWARN("**** put back the gtg unit tests!");
    gtg0();
    // gtg1();


}