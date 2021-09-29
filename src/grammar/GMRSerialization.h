#pragma once

#include <memory>
#include <string>

struct json_t;
class StochasticGrammar;
class StochasticProductionRule;
class StochasticProductionRuleEntry;

using StochasticGrammarPtr = std::shared_ptr<StochasticGrammar>;
using StochasticProductionRulePtr = std::shared_ptr<StochasticProductionRule>;
using StochasticProductionRuleEntryPtr = std::shared_ptr<StochasticProductionRuleEntry>;

class GMRSerialization {
public:
   static StochasticGrammarPtr readGrammarFile(const std::string& path);
private:
    static StochasticGrammarPtr readGrammar(const std::string&);
    static StochasticGrammarPtr readGrammar(json_t*);
    static StochasticProductionRulePtr readRule(json_t*);
    static StochasticProductionRuleEntryPtr readRuleEntry(json_t*);
};
