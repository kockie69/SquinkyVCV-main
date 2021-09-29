#include "StochasticGrammar2.h"
#include "StochasticNote.h"
#include "StochasticProductionRule.h"
#include "asserts.h"

static void test0() {
    StochasticDisplayNote n(StochasticDisplayNote::Durations::half);
    assertEQnp(n.duration, StochasticDisplayNote::Durations::half);
    assertEQnp(n.tuple, StochasticDisplayNote::Tuples::none);

    StochasticDisplayNote n2(StochasticDisplayNote::Durations::eighth, StochasticDisplayNote::Tuples::triplet);
    assertEQnp(n2.duration, StochasticDisplayNote::Durations::eighth);
    assertEQnp(n2.tuple, StochasticDisplayNote::Tuples::triplet);
}

static void testNoteDuration(const StochasticDisplayNote::Durations dur, double expectedDur) {
    StochasticDisplayNote n(dur);
    assertEQ(n.timeDuration(), expectedDur);
}

static void testNoteDurations() {
    testNoteDuration(StochasticDisplayNote::Durations::half, .5);
    testNoteDuration(StochasticDisplayNote::Durations::quarter, .25);
    testNoteDuration(StochasticDisplayNote::Durations::eighth, .125);
    testNoteDuration(StochasticDisplayNote::Durations::sixteenth, .25 * .25);
    testNoteDuration(StochasticDisplayNote::Durations::thirtysecond, .5 * .25 * .25);
}

static void test1() {
    StochasticProductionRuleEntryPtr entry = StochasticProductionRuleEntry::make();
    entry->rhsProducedNotes.push_back(StochasticNote::eighth());
    entry->rhsProducedNotes.push_back(StochasticNote::eighth());
    entry->probability = .1;
    assert(entry->isValid());

    StochasticProductionRule rule(StochasticNote::quarter());
    rule.addEntry(entry);
    assert(rule.isValid());
}

class TestEvaluator : public StochasticProductionRule::EvaluationState {
public:
    TestEvaluator(AudioMath::RandomUniformFunc xr) : StochasticProductionRule::EvaluationState(xr) {
    }

    void writeSymbol(const StochasticNote& sym) override {
        notes.push_back(sym);
    }

    size_t getNumSymbols() {
        return notes.size();
    }

private:
    std::vector<StochasticNote> notes;
};

#if 0
static ConstStochasticGrammarPtr getGrammar() {
    StochasticGrammarPtr gmr = std::make_shared<StochasticGrammar>();

    auto rootRule = std::make_shared<StochasticProductionRule>(StochasticNote::half());
    assert(rootRule->isRuleValid());
    auto entry = StochasticProductionRuleEntry::make();
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->probabilty = .5;
    rootRule->addEntry(entry);
    assert(rootRule->isRuleValid());

    gmr->addRootRule(rootRule);
    //gmr.addRootRule(rootRule);
   // auto rule = std::make_shared<StochasticProductionRule>(StochasticNote(StochasticNote::Durations::half));


    return gmr;
}
#endif


static void testGetLHS() {
    // auto gmr = getGrammar();
    auto gmr = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::simple);
    auto lhs = gmr->getAllLHS();

    // test grammer only has a single production rule
    assertEQ(lhs.size(), 1);
}

static void testNote2Text() {
    {
        StochasticNote n = StochasticNote::half();
        assertEQ(n.toText(), "1/2");
    }
    {
        StochasticNote n = StochasticNote::quarter();
        assertEQ(n.toText(), "1/4");
    }
    {
        StochasticNote n = StochasticNote::eighth();
        assertEQ(n.toText(), "1/8");
    }
    {
        StochasticNote n = StochasticNote::sixteenth();
        assertEQ(n.toText(), "1/16");
    }
    {
        StochasticNote n = StochasticNote::dotted_half();
        assertEQ(n.toText(), ".-1/2");
    }
    {
        StochasticNote n = StochasticNote::dotted_quarter();
        assertEQ(n.toText(), ".-1/4");
    }
    {
        StochasticNote n = StochasticNote::dotted_eighth();
        assertEQ(n.toText(), ".-1/8");
    }
    {
        StochasticNote n = StochasticNote::dotted_sixteenth();
        assertEQ(n.toText(), ".-1/16");
    }
    {
        StochasticNote n = StochasticNote::trip_half();
        assertEQ(n.toText(), "1/2(3)");
    }
    {
        StochasticNote n = StochasticNote::trip_quarter();
        assertEQ(n.toText(), "1/4(3)");
    }
    {
        StochasticNote n = StochasticNote::trip_eighth();
        assertEQ(n.toText(), "1/8(3)");
    }
    {
        StochasticNote n = StochasticNote::trip_sixteenth();
        assertEQ(n.toText(), "1/16(3)");
    }
}

static void testNoteFromText() {
    {
        StochasticNote n = StochasticNote::fromString("h");
        assertEQ(n.duration, StochasticNote::ppq * 2);
    }
    {
        StochasticNote n = StochasticNote::fromString("q");
        assertEQ(n.duration, StochasticNote::ppq);
    }
    {
        StochasticNote n = StochasticNote::fromString("e");
        assertEQ(n.duration, StochasticNote::ppq / 2);
    }
    {
        StochasticNote n = StochasticNote::fromString("x");
        assertEQ(n.duration, StochasticNote::ppq / 4);
    }
    {
        StochasticNote n = StochasticNote::fromString("s");
        assertEQ(n.duration, StochasticNote::ppq / 4);
    }
    {
        StochasticNote n = StochasticNote::fromString(".h");
        assertEQ(n.duration, StochasticNote::ppq * 3);
    }
    {
        StochasticNote n = StochasticNote::fromString(".q");
        assertEQ(n.duration, StochasticNote::ppq * 3 / 2);
    }
    {
        StochasticNote n = StochasticNote::fromString(".e");
        assertEQ(n.duration, StochasticNote::ppq * 3 / 4);
    }
    {
        StochasticNote n = StochasticNote::fromString(".x");
        assertEQ(n.duration, StochasticNote::ppq * 3 / 8);
    }
    {
        StochasticNote n = StochasticNote::fromString(".s");
        assertEQ(n.duration, StochasticNote::ppq * 3 / 8);
    }
    {
        StochasticNote n = StochasticNote::fromString("3h");
        assertEQ(n.duration, StochasticNote::ppq * 4 / 3);
    }
    {
        StochasticNote n = StochasticNote::fromString("3q");
        assertEQ(n.duration, StochasticNote::ppq * 2 / 3);
    }
    {
        StochasticNote n = StochasticNote::fromString("3e");
        assertEQ(n.duration, StochasticNote::ppq * 1 / 3);
    }
    {
        StochasticNote n = StochasticNote::fromString("3x");
        assertEQ(n.duration, StochasticNote::ppq / 6);
    }
    {
        StochasticNote n = StochasticNote::fromString("3s");
        assertEQ(n.duration, StochasticNote::ppq / 6);
    }
}

static void testLHS() {
    auto gmr = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::demo);
    auto lhs = gmr->getAllLHS();
    assertEQ(lhs.size(), 3);

    int x = 100000000;
    for (auto note : lhs) {
        const int d = note.duration;
        assertLT(d, x);
        x = d;
    }
}

static void testDemos() {
    auto gmr = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::demo);
    assert(gmr->isValid());
    gmr->_dump();

    gmr = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::quarters);
    assert(gmr->isValid());

    gmr = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::simple);
    assert(gmr->isValid());

     gmr = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::x25);
    assert(gmr->isValid());
}

void testStochasticGrammar2() {
    test0();
    testNoteDurations();
    test1();
  //  testGrammar1();
    testGetLHS();
    testNote2Text();
    testNoteFromText();
    testLHS();
    testDemos();
}