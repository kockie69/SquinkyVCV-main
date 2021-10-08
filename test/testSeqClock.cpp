
#include "OneShot.h"
#include "SeqClock.h"
#include "SeqClock2.h"
#include "asserts.h"

static void testOneShotInit() {
    OneShot o;
    // should always be fired, until triggered
    assert(o.hasFired());
    o.setDelayMs(1);
    assert(o.hasFired());
    o.setSampleTime(1.f / 44100.f);
    assert(o.hasFired());
    o.set();
    assert(!o.hasFired());
}

static void testOneShot2Ms() {
    OneShot o;
    o.setSampleTime(.001f);  // sample rate 1k
    o.setDelayMs(2);         // delay 2ms
    assert(o.hasFired());
    o.set();
    assert(!o.hasFired());

    o.step();  // 1 ms.
    assert(!o.hasFired());

    o.step();  // 2 ms.
    assert(o.hasFired());

    for (int i = 0; i < 10; ++i) {
        o.step();
        assert(o.hasFired());
    }

    o.set();
    assert(!o.hasFired());

    o.step();  // 1 ms.
    assert(!o.hasFired());

    o.step();  // 2 ms.
    assert(o.hasFired());
}

//  SeqClock::update(int samplesElapsed, float externalClock, float runStop, float reset)
// test internal clock
#if 0  // no more internal
static void testClockInternal0()
{
    assert(false);      // internal has changes
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    SeqClock ck;
    ck.setup(SeqClock::ClockRate::Internal, 120, sampleTime);       // internal clock

    // now clock by one second
    SeqClock::ClockResults results = ck.update(sampleRateI, 0, true, 0);

    // quarter note = half second at 120,
    // so one second = 2q
    assertEQ(results.totalElapsedTime, 2.0);
    assert(!results.didReset);

    results = ck.update(sampleRateI, 0, true, 0);
    assertEQ(results.totalElapsedTime, 4.0);

    ck.reset(true);
    ck.setup(SeqClock::ClockRate::Internal, 240, sampleTime);       // internal clock
    results = ck.update(sampleRateI * 10, 0, true, 0);
    assertEQ(results.totalElapsedTime, 40);
  //  assertGT(results.totalElapsedTime, 38.9);
 //   assertLE(results.totalElapsedTime, 40);
// with original reset this was 40. new reset clock logic maybe doesn't work right with internal
    
    assert(!results.didReset);
}
#endif

static void testClockExt(SeqClock::ClockRate rate, double metricTimePerClock) {
    SeqClock ck;
    ck.setup(rate, 120, 100);  // internal clock

    SeqClock::ClockResults results;

    // low clock, nothing will happen
    for (int i = 0; i < 10; ++i) {
        results = ck.update(55, 0, true, 0);    // low clock
        assertLT(results.totalElapsedTime, 0);  // still waiting for first
    }

    // first high clock, will start by advancing to time 0
    results = ck.update(55, 10, true, 0);
    assertEQ(results.totalElapsedTime, 0);

    // now another low clock
    results = ck.update(55, 0, true, 0);
    assertEQ(results.totalElapsedTime, 0);

    // real high clock: count how much metric time comes back
    results = ck.update(55, 10, true, 0);
    assertEQ(results.totalElapsedTime, metricTimePerClock);
}

static void testClockExt1() {
    testClockExt(SeqClock::ClockRate::Div1, 1.0);
    testClockExt(SeqClock::ClockRate::Div2, 1.0 / 2.0);
    testClockExt(SeqClock::ClockRate::Div4, 1.0 / 4.0);
    testClockExt(SeqClock::ClockRate::Div8, 1.0 / 8.0);
    testClockExt(SeqClock::ClockRate::Div16, 1.0 / 16.0);
}

template <typename TClock>
static void testClockExtEdge() {
    auto rate = TClock::ClockRate::Div1;
    const double metricTimePerClock = 1;
    TClock ck;
    typename TClock::ClockResults results;
    ck.setup(rate, 120, 100);  // internal clock

    // send one clock (first low)
    for (int i = 0; i < 10; ++i) {
        results = ck.update(55, 0, true, 0);    // low clock
        assertLT(results.totalElapsedTime, 0);  // in start up
    }

    // then high once to start at time zero
    results = ck.update(55, 10, true, 0);
    assertEQ(results.totalElapsedTime, 0);

    // then low once to get ready for next clock
    results = ck.update(55, 0, true, 0);
    assertEQ(results.totalElapsedTime, 0);

    // then high once
    results = ck.update(55, 10, true, 0);
    assertEQ(results.totalElapsedTime, metricTimePerClock);

    // then high some more
    for (int i = 0; i < 10; ++i) {
        results = ck.update(55, 10, true, 0);  // low clock
        assertEQ(results.totalElapsedTime, metricTimePerClock);
    }

    // low more
    for (int i = 0; i < 10; ++i) {
        results = ck.update(55, 0, true, 0);  // low clock
        assertEQ(results.totalElapsedTime, metricTimePerClock);
    }

    // then high some more
    for (int i = 0; i < 10; ++i) {
        results = ck.update(55, 10, true, 0);  // low clock
        assertEQ(results.totalElapsedTime, 2 * metricTimePerClock);
    }
}

#if 0  // no more inernal. do we need a new test?
static void testClockInternalRunStop()
{
    assert(false);  // no more internal
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    SeqClock ck;
    ck.setup(SeqClock::ClockRate::Internal, 120, sampleTime);       // internal clock

    // now clock by one second
    SeqClock::ClockResults results = ck.update(sampleRateI, 0, true, 0);

    // quarter note = half second at 120,
    // so one second = 2q
    assertEQ(results.totalElapsedTime, 2.0);

    // now clock stopped, should not run
    results = ck.update(sampleRateI, 0, 0, 0);
    assertEQ(results.totalElapsedTime, 2.0);

     // now on again, should run
    results = ck.update(sampleRateI, 0, true, 0);
    assertEQ(results.totalElapsedTime, 4.0);
}
#endif

template <typename TClock>
static void testClockChangeWhileStopped() {
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    TClock ck;
    ck.setup(TClock::ClockRate::Div1, 120, sampleTime);  // external clock

    // call with clock low,high,low whole running
    // to get to time zero, ready for first
    typename TClock::ClockResults results;
    ck.update(sampleRateI, 0, true, 0);
    ck.update(sampleRateI, 10, true, 0);
    results = ck.update(sampleRateI, 0, true, 0);

    assertEQ(results.totalElapsedTime, 0);

    // now stop
    results = ck.update(sampleRateI, 0, false, 0);
    assertEQ(results.totalElapsedTime, 0);

    // raise clock while stopped
    for (int i = 0; i < 10; ++i) {
        results = ck.update(sampleRateI, 10, false, 0);
    }
    assertEQ(results.totalElapsedTime, 0);

    // now run. see if we catch the edge
    results = ck.update(sampleRateI, 10, true, 0);
    assertEQ(results.totalElapsedTime, 1);
}

#if 0  // no inernal, do we need new test?
static void testSimpleReset()
{
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    SeqClock ck;
    SeqClock::ClockResults results;
    ck.setup(SeqClock::ClockRate::Internal, 120, sampleTime);       // internal clock

    // one second goes by at 120 -> half note
    results = ck.update(sampleRateI, 0, true, 0);
    assert(!results.didReset);
    assertEQ(results.totalElapsedTime, 2.f);

    // now reset. should send reset, and set us back to zero, but
    // not suppress us
    results = ck.update(sampleRateI, 0, true, 10);
    assert(results.didReset);
    assertEQ(results.totalElapsedTime, 2.f);

    results = ck.update(sampleRateI, 0, true, 10);
    assert(!results.didReset);

    results = ck.update(sampleRateI, 0, true, 10);
    assert(!results.didReset);

    results = ck.update(sampleRateI, 0, true, 0);
    assert(!results.didReset);
}
#endif

template <typename TClock>
static void testSimpleResetIgnoreClock() {
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    TClock ck;
    SeqClock::ClockResults results;
    ck.setup(SeqClock::ClockRate::Div16, 120, sampleTime);  // external clock tempo 120

    // run external clock high
    results = ck.update(sampleRateI, 10, true, 0);
    assert(!results.didReset);
    const double t0 = results.totalElapsedTime;

    // clock low and high
    ck.update(sampleRateI, 0, true, 0);
    results = ck.update(sampleRateI, 10, true, 0);
    const double t1 = results.totalElapsedTime;

    assertGT(t1, t0);  // we are clocking now

    // now reset
    // Note that this resets clock while it is running, then runs
    results = ck.update(sampleRateI, 10, true, 10);
    assert(results.didReset);
    assertLT(results.totalElapsedTime, 0);  // reset should set clock back to waiting

    results = ck.update(sampleRateI, 0, true, 0);
    assert(!results.didReset);

    //   ClockResults update(int samplesElapsed, float externalClock, bool runStop, float reset);

    // clock should be locked out now
    results = ck.update(1, 10, true, 0);
    assert(!results.didReset);
}

template <typename TClock>
static double getExpectedResetTime() {
    assert(false);
    return -10;
}

template <>
//static double getExpectedResetTime<SeqClock>() {
double getExpectedResetTime<SeqClock>() {
    return -1;
}

template <>
double getExpectedResetTime<SeqClock2>() {
    return 0;
}

template <typename TClock>
static void testResetIgnoreClock() {
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    int samplesInOneMs = int(44100.f / 1000.f);

    TClock ck;
    typename TClock::ClockResults results;
    ck.setup(TClock::ClockRate::Div1, 120, sampleTime);  // external clock = quarter

    // run external clock high
    results = ck.update(sampleRateI, 10, true, 0);
    assert(!results.didReset);
    const double t0 = results.totalElapsedTime;

    // clock low and high
    ck.update(sampleRateI, 0, true, 0);
    results = ck.update(sampleRateI, 10, true, 0);
    const double t1 = results.totalElapsedTime;

    assertClose(t1 - t0, 1, .0001);  // quarter note elapsed from one clock edge

    // now reset
    results = ck.update(1, 10, true, 10);
    assert(results.didReset);
    assertLE(results.totalElapsedTime, getExpectedResetTime<TClock>());  // reset should set clock back to waiting

    //   ClockResults update(int samplesElapsed, float externalClock, bool runStop, float reset);

    int errorMargin = 10;
    // step for a little under one ms

    for (int i = 0; i < (samplesInOneMs - errorMargin); ++i) {
        results = ck.update(1, 0, true, 0);
        assertLE(results.totalElapsedTime, getExpectedResetTime<TClock>());
    }

    // this clock should be ignored
    results = ck.update(1, 10, true, 0);
    assertLE(results.totalElapsedTime, getExpectedResetTime<TClock>());

    // step for a little more with clock low
    for (int i = 0; i < 2 * errorMargin; ++i) {
        results = ck.update(1, 0, true, 0);
        assertLE(results.totalElapsedTime, getExpectedResetTime<TClock>());
    }

    // this clock should NOT be ignored
    results = ck.update(sampleRateI, 10, true, 0);
    assertEQ(results.totalElapsedTime, getExpectedResetTime<TClock>() + 1);  // first clock after reset advance to start
                                                                             // assertClose(results.totalElapsedTime, 1, .000001);
}

template <typename TClock>
static void testRates() {
    const int num = int(TClock::ClockRate::NUM_CLOCKS);
    auto labels = TClock::getClockRates();
    assert(num == labels.size());
    for (std::string label : labels) {
        assert(!label.empty());
    }
}

template <typename TClock>
static void testNoNoteAfterReset() {
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    TClock ck;
    typename TClock::ClockResults results;
    ck.setup(TClock::ClockRate::Div1, 120, sampleTime);  // external clock = quarter

    //   ClockResults update(int samplesElapsed, float externalClock, bool runStop, float reset)

    // clock it a bit

    for (int j = 0; j < 10; ++j) {
        results = ck.update(100, 0, true, 0);
        results = ck.update(100, 10, true, 0);
    }

    // stop it
    results = ck.update(1, 0, false, 0);
    assert(!results.didReset);

    // reset it
    results = ck.update(1, 0, false, 10);
    assert(results.didReset);

    // after reset we should output time before start, so that when we finally start we play the first note
    assertLT(results.totalElapsedTime, getExpectedResetTime<TClock>() + 1);

    // while we are stopped, time should still not pass, even if clocked
    for (int i = 0; i < 100; ++i) {
        results = ck.update(100, 0, false, 0);
        assert(!results.didReset);
        assertLT(results.totalElapsedTime, getExpectedResetTime<TClock>() + 1);

        results = ck.update(100, 10, false, 0);
        assert(!results.didReset);
        assertLT(results.totalElapsedTime, getExpectedResetTime<TClock>() + 1);
    }

    // now let it run (but no clock)
    for (int i = 0; i < 100; ++i) {
        results = ck.update(100, 0, true, 0);
        assert(!results.didReset);
        assertLT(results.totalElapsedTime, getExpectedResetTime<TClock>() + 1);
    }

    // now clock it - should go to zero

    printf("finish this reset test\n");
    // assert(false);      // finish me
}

template <typename TClock>
static void testRunGeneratesClock() {
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    TClock ck;  // freq new clock
    typename TClock::ClockResults results;
    ck.setup(TClock::ClockRate::Div1, 120, sampleTime);  // external clock = quarter

    // clock with run off, clock high. Nothing happens
    for (int j = 0; j < 10; ++j) {
        results = ck.update(100, 10, false, 0);
        assert(!results.didReset);
        assertLT(results.totalElapsedTime, 0);
    }

    // now run. rising run signal should gen a clock with clock high
    results = ck.update(1, 10, true, 0);
    assert(!results.didReset);
    assertEQ(results.totalElapsedTime, 0);
}

template <typename TClock>
static void testResetRetriggersClock() {
    const int sampleRateI = 44100;
    const float sampleRate = float(sampleRateI);
    const float sampleTime = 1.f / sampleRate;

    TClock ck;  // freq new clock
    typename TClock::ClockResults results;
    ck.setup(TClock::ClockRate::Div1, 120, sampleTime);  // external clock = quarter

    // clock with run off, clock high. Nothing happens
    for (int j = 0; j < 10; ++j) {
        results = ck.update(100, 10, false, 0);
        assert(!results.didReset);
        assertLT(results.totalElapsedTime, 0);
    }

    // now run. rising run signal should gen a clock with clock high
    results = ck.update(1, 10, true, 0);
    assert(!results.didReset);
    assertEQ(results.totalElapsedTime, 0);

    // stay high should not generate more clocks
    for (int j = 0; j < 10; ++j) {
        results = ck.update(100, 10, true, 0);
        assert(!results.didReset);
        assertEQ(results.totalElapsedTime, 0);
    }

    // reset should re-trigger clock, but only after lock-out interval
    results = ck.update(1, 10, true, 10);
    assert(results.didReset);
    assertLT(results.totalElapsedTime, getExpectedResetTime<TClock>() + 1);

    // now wait through the lock-out interval
    const int lockOutSamples = 1000;  // TODO: expose magic num from clock
    results = ck.update(lockOutSamples, 10, true, 0);
    assertEQ(results.totalElapsedTime, getExpectedResetTime<TClock>() + 1);
}

static void testClockRates2() {
    SeqClock2 test(0);
    for (int i = 0; i < int(SeqClock2::ClockRate::NUM_CLOCKS); ++i) {
        auto clockRate = SeqClock2::ClockRate(i); 
        test.setup(clockRate , -1000000, 1.f / 44100.f);

        auto x = SeqClock2::clockRate2Div(clockRate);
        assertGE(x, 1);
    }

    size_t rates = SeqClock2::getClockRates().size();
    assertEQ(rates, size_t(SeqClock2::ClockRate::NUM_CLOCKS));
}

template <typename T>
void testSeqClock2() {
    testClockExtEdge<T>();
    testClockChangeWhileStopped<T>();
    testResetIgnoreClock<T>();
    testRates<T>();
    testNoNoteAfterReset<T>();
    testRunGeneratesClock<T>();
    testResetRetriggersClock<T>();
    //  testClockExt1();
}

void testSeqClock() {
    testSeqClock2<SeqClock>();
    testSeqClock2<SeqClock2>();
    testOneShotInit();
    testOneShot2Ms();;
    testClockExt1();
    testClockRates2();
    printf("let's make testClockExternalRunStop\n");

    //testSimpleReset();
    // testSimpleResetIgnoreClock();
    printf("(eventually) put back clock reset tests\n");  // since I disable clock lockout after reset
                                                          // these tests are wrong
}