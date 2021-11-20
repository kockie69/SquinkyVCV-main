#include "StochasticProductionRule.h"
#include "StochasticGrammar2.h"



StochasticProductionRulePtr StochasticGrammar::getRule(const StochasticNote& n) const {
    auto it = rules.find(n);
    return (it == rules.end()) ? nullptr : it->second;
}

StochasticProductionRulePtr StochasticGrammar::getRootRule() const {
    auto rootIter = rules.begin();
    if (rootIter == rules.end()) {
        return nullptr;
    }

    StochasticProductionRulePtr ret = rootIter->second;

    return ret;
}

void StochasticGrammar::addRule(StochasticProductionRulePtr rule) {
    const StochasticNote& n = rule->lhs;
    assert(getRule(n) == nullptr);  // we don't let rules get added twice

    std::pair<StochasticNote, StochasticProductionRulePtr> value(n, rule);
    rules.insert(value);
}

std::vector<StochasticNote> StochasticGrammar::getAllLHS() const {
    std::vector<StochasticNote> ret;

    std::set<StochasticNote> test;
    for (auto rule : rules) {
        StochasticNote note = rule.first;
        assert(test.find(note) == test.end());
        test.insert(note);
        ret.push_back(note);
    }

    return ret;
}

bool StochasticGrammar::isValid() const {
    int dur = 1000000000;
    for (auto r : rules) {
        ConstStochasticProductionRulePtr rule = r.second;
        bool b = rule->isValid();
        if (!b) {
            //SQWARN("invalid rule");
            return false;
        }
        const int duration = rule->lhs.duration;
        if (duration >= dur) {
            //SQWARN("duration not desc %d vs %d", duration, dur);
            return false;
        }
        dur = duration;
    }
    auto root = getRootRule();
    if (!root) {
        //SQWARN("grammar has no root");
        return false;
    }
    const int rootDuration = root->lhs.duration;
    for (auto r : rules) {
        ConstStochasticProductionRulePtr rule = r.second;
        const int duration = rule->lhs.duration;
        if (duration > rootDuration) {
            //SQWARN("root isn't longest rule");
            return false;
        }
    }
    return true;
}

void StochasticGrammar::_dump() const {
    //SQINFO("***** dumping grammar");

    for (auto r : rules) {
        ConstStochasticProductionRulePtr rule = r.second;
        rule->_dump("next rule in gmr");
    }
    //SQINFO("***** done dumping grammar");
}

StochasticGrammarPtr StochasticGrammar::getDemoGrammar(DemoGrammar g) {
    StochasticGrammarPtr ret;
    switch (g) {
        case DemoGrammar::simple:
            ret = getDemoGrammarSimple();
            break;
        case DemoGrammar::demo:
            ret = getDemoGrammarDemo();
            break;
        case DemoGrammar::quarters:
            ret = getDemoGrammarQuarters();
            break;
        case DemoGrammar::x25:
            ret = getDemoGrammar25();
            break;
    }

    assert(ret);
    assert(ret->isValid());
    return ret;
}

StochasticGrammarPtr StochasticGrammar::getDemoGrammarDemo() {
    auto grammar = std::make_shared<StochasticGrammar>();

    // half rules. splits to quarter
    {
        auto rootRule = std::make_shared<StochasticProductionRule>(StochasticNote::half());

        // half rules
        auto entry = StochasticProductionRuleEntry::make();
        entry->rhsProducedNotes.push_back(StochasticNote::quarter());
        entry->rhsProducedNotes.push_back(StochasticNote::quarter());
        entry->probability = .6;
        rootRule->addEntry(entry);

        assert(rootRule->isValid());
        grammar->addRule(rootRule);

        entry = StochasticProductionRuleEntry::make();
        entry->rhsProducedNotes.push_back(StochasticNote::quarter());
        entry->rhsProducedNotes.push_back(StochasticNote::eighth());
        entry->rhsProducedNotes.push_back(StochasticNote::eighth());
        entry->probability = .3;
        rootRule->addEntry(entry);
    }

    // quarter rules. splits to eighth
    {
        auto rule = std::make_shared<StochasticProductionRule>(StochasticNote::quarter());

        auto entry = StochasticProductionRuleEntry::make();
        entry->rhsProducedNotes.push_back(StochasticNote::eighth());
        entry->rhsProducedNotes.push_back(StochasticNote::eighth());
        entry->probability = .7;
        rule->addEntry(entry);
        grammar->addRule(rule);
    }

    // eighth rule
    {
        auto rule = std::make_shared<StochasticProductionRule>(StochasticNote::eighth());

        auto entry = StochasticProductionRuleEntry::make();
        entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
        entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
        entry->probability = .5;
        rule->addEntry(entry);
        grammar->addRule(rule);
    }

    return grammar;
}

StochasticGrammarPtr StochasticGrammar::getDemoGrammarSimple() {
    auto grammar = std::make_shared<StochasticGrammar>();

    auto rootRule = std::make_shared<StochasticProductionRule>(StochasticNote::half());
    assert(rootRule->isValid());
    auto entry = StochasticProductionRuleEntry::make();
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->probability = .5;
    rootRule->addEntry(entry);
    assert(rootRule->isValid());
    grammar->addRule(rootRule);
    return grammar;
}

StochasticGrammarPtr StochasticGrammar::getDemoGrammarQuarters() {
    auto grammar = std::make_shared<StochasticGrammar>();

    auto rootRule = std::make_shared<StochasticProductionRule>(StochasticNote::half());
    assert(rootRule->isValid());
    auto entry = StochasticProductionRuleEntry::make();
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->probability = 1;
    rootRule->addEntry(entry);
    assert(rootRule->isValid());
    grammar->addRule(rootRule);
    return grammar;
}

StochasticGrammarPtr StochasticGrammar::getDemoGrammar25() {
    auto grammar = std::make_shared<StochasticGrammar>();

    auto rootRule = std::make_shared<StochasticProductionRule>(StochasticNote::half());
    assert(rootRule->isValid());


    // 0.. .25 -> quarters
    auto entry = StochasticProductionRuleEntry::make();
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->probability = .25;
    rootRule->addEntry(entry);

    // .25.. .50 -> eighths
    entry = StochasticProductionRuleEntry::make();
    entry->rhsProducedNotes.push_back(StochasticNote::eighth());
    entry->rhsProducedNotes.push_back(StochasticNote::eighth());
    entry->rhsProducedNotes.push_back(StochasticNote::eighth());
    entry->rhsProducedNotes.push_back(StochasticNote::eighth());
    entry->probability = .25;
    rootRule->addEntry(entry);

    // .50 .. .75 -> sixteenth
    entry = StochasticProductionRuleEntry::make();
    entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
    entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
    entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
    entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
    entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
    entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
    entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
    entry->rhsProducedNotes.push_back(StochasticNote::sixteenth());
    entry->probability = .25;
    rootRule->addEntry(entry);

    // leaving .75 .. 1 for half

    assert(rootRule->isValid());
    grammar->addRule(rootRule);
    return grammar;
}
