
#include "StochasticProductionRule.h"

#include <set>

#include "SqLog.h"
#include "StochasticGrammar2.h"


std::vector<StochasticNote>* StochasticProductionRule::_evaluateRule(const StochasticProductionRule& rule, float random) {
    assert(random >= 0 && random <= 1);
    assert(&rule != nullptr);

    double probabilityRange = 0;
    for (auto it = rule.entries.begin(); it != rule.entries.end(); ++it) {
        StochasticProductionRuleEntryPtr entry = *it;
        
        probabilityRange += entry->probability;
        if ((random - probabilityRange) < 0) {
            return &entry->rhsProducedNotes;
        }
    }
    // no rule fired, will terminate
    return nullptr;
}

#if 0 // the old version
std::vector<StochasticNote>* StochasticProductionRule::_evaluateRule(const StochasticProductionRule& rule, float random) {
    assert(random >= 0 && random <= 1);
    assert(&rule != nullptr);

    for (auto it = rule.entries.begin(); it != rule.entries.end(); ++it) {
        StochasticProductionRuleEntryPtr entry = *it;
        if (entry->probability >= random) {
            return &entry->rhsProducedNotes;
        }
    }
    // no rule fired, will terminate
    return nullptr;
}
#endif

void StochasticProductionRule::evaluate(EvaluationState& es, const StochasticProductionRulePtr ruleToEval) {
    //SQINFO("StochasticProductionRule::evaluate %p", ruleToEval.get());
    assert(ruleToEval);
    assert(ruleToEval->isValid());

    // ruleToEval->_dump("evaluating");
    auto result = _evaluateRule(*ruleToEval, es.r());
    if (!result)  // request to terminate recursion
    {
        es.writeSymbol(ruleToEval->lhs);
        //SQINFO("evaluate found terminal, writing");
    } else {
        //SQINFO("expanding %d into %d notes", ruleToEval->lhs.duration, (int)result->size());
        for (auto note : *result) {
            auto rule = es.grammar->getRule(note);
            if (!rule) {
                //SQINFO("found a terminal in the results  %d", note.duration);
                es.writeSymbol(note);
            } else {
                evaluate(es, rule);
            }
        }
    }
}

bool StochasticProductionRule::isValid() const {
    //SQINFO("rule, is valid");
    if (this->lhs.duration < 1) {
        //SQINFO("error: zero duration rule");
        return false;
    }
    for (auto entry : entries) {
        if (!entry->isValid()) {
            //SQINFO("error: bad entry:");
            entry->_dump();
            entry->isValid();
            return false;
        }
        //SQINFO("entry=%d, lhs=%d", entry->duration(), lhs.duration);
        if (entry->duration() != lhs.duration) {
            //SQINFO("error: entry mismatch %d vs %d", lhs.duration, entry->duration());
            return false;
        }
    }

    return true;
}

void StochasticProductionRule::_dump(const char* title) const {
    assert(this);
    //SQINFO("---- dump rule %s, lhs dur=%d", title, this->lhs.duration);
    for (auto entry : this->entries) {
        // StochasticProductionRuleEntryPtr e = entry;
        entry->_dump();
    }
    //SQINFO("---- end dump");
}

bool StochasticProductionRule::isGrammarValidSub(const StochasticGrammar& grammar, StochasticProductionRulePtr rule, std::set<size_t>& rulesHit) {
    assert(rule);

    //SQINFO("enteer sub rule = %p", rule.get());
    if (!rule->isValid()) {
        //SQINFO("invalid rule");
        rule->_dump("invalid");
        return false;
    }

    size_t x = (size_t)(void*)rule.get();
    rulesHit.insert(x);
    auto& entries = rule->entries;
    for (auto entry : entries) {
        auto notes = entry->rhsProducedNotes;
        for (auto note : notes) {
            auto nextRule = grammar.getRule(note);
            if (!nextRule) {
                //SQINFO("found a note entry with no rule %d. must be a terminal", note.duration);
                return true;
            }
            if (nextRule.get() == rule.get()) {
                //SQINFO("grammar has loop");
                return false;
            }
            if (!isGrammarValidSub(grammar, nextRule, rulesHit)) {
                //SQINFO("found a bad rule");
                return false;
            }
        }
    }
    return true;
}

bool StochasticProductionRule::isGrammarValid(const StochasticGrammar& grammar) {
    auto nextRule = grammar.getRootRule();
    if (!nextRule) {
        //SQINFO("grammar has no root");
        return false;
    }

    std::set<size_t> rulesHit;
    bool ok = isGrammarValidSub(grammar, nextRule, rulesHit);
    if (!ok) {
        return false;
    }

    if (rulesHit.size() != grammar.size()) {
        const int gsize = int(grammar.size());
        //SQINFO("didn't hit all rules g=%d found%d", gsize, int(rulesHit.size()));
        return false;
    }

    return true;
}
