#pragma once

#include <assert.h>

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "SqLog.h"
#include "StochasticNote.h"

class StochasticProductionRule;

class StochasticGrammar;

using StochasticProductionRulePtr = std::shared_ptr<StochasticProductionRule>;
using StochasticGrammarPtr = std::shared_ptr<StochasticGrammar>;
using ConstStochasticGrammarPtr = std::shared_ptr<const StochasticGrammar>;

/**
 * class that holds an entire grammar
 */
class StochasticGrammar {
public:
    ~StochasticGrammar() { /*//SQINFO("dtor of StochasticGrammar");*/ }
    StochasticProductionRulePtr getRule(const StochasticNote&) const;
    StochasticProductionRulePtr getRootRule() const;
    void addRule(StochasticProductionRulePtr rule);
#if 0
    void addRootRule(StochasticProductionRulePtr rule) {
        assert(!rootRule);
        rootRule = rule;
        addRule(rule);
    }
#endif

    std::vector<StochasticNote> getAllLHS() const;

    enum class DemoGrammar {
        simple,
        demo,
        quarters,
        x25     // 25% 8 X 16, 4Xe, 2xq, 1Xh
    };
    static StochasticGrammarPtr getDemoGrammar(DemoGrammar);
    size_t size() const { return rules.size(); }
    bool isValid() const;
    void _dump() const;

private:
    // DO WE REALLY NEED MULTIMAP? maybe in future?
    // TODO: make unordered work
    //  std::unordered_multimap<StochasticNote, StochasticProductionRulePtr> map;
    std::multimap<StochasticNote, StochasticProductionRulePtr> rules;
    // StochasticProductionRulePtr rootRule;

    static StochasticGrammarPtr getDemoGrammarSimple();
    static StochasticGrammarPtr getDemoGrammarDemo();
    static StochasticGrammarPtr getDemoGrammarQuarters();
    static StochasticGrammarPtr getDemoGrammar25();
};
