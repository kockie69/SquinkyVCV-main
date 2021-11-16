
#include "rack.hpp"
#include "GMRSerialization.h"
#include "SqStream.h"

#include <assert.h>

#include "StochasticGrammar2.h"
#include "StochasticProductionRule.h"


StochasticGrammarPtr GMRSerialization::readGrammarFile(const std::string& path) {
    // json_t *json_load_file(const char *path, size_t flags, json_error_t *error) JANSSON_ATTRS(warn_unused_result);
    json_error_t error;
    json_t* theJson = json_load_file(path.c_str(), 0, &error);
    if (theJson) {
        return readGrammar(theJson);
    }
    SqStream s;
    s.add("JSON parsing error at ");
    s.add(error.line);
    s.add(":");
    s.add(error.column);
    s.add(" ");
    s.add(error.text);
 //   fclose(file);
    WARN("%s", s.str().c_str());

    return nullptr;
}

StochasticGrammarPtr GMRSerialization::readGrammar(const std::string& s) {
    // json_loads(const char *input, size_t flags, json_error_t *error) JANSSON_ATTRS(warn_unused_result);
    json_error_t error;
    json_t* theJson = json_loads(s.c_str(), 0, &error);
    assert(theJson);
    if (theJson) {
        return readGrammar(theJson);
    }
    return nullptr;
}

static const char* _rules = "rules";
static const char* _lhs = "lhs";
static const char* _rhs = "rhs";
static const char* _p = "p";
static const char* _entries = "entries";

StochasticProductionRuleEntryPtr GMRSerialization::readRuleEntry(json_t* entryJson) {
    // get probability number
    json_t* prob = json_object_get(entryJson, _p);
    if (!prob) {
        WARN("rule has no probability");
        return nullptr;
    }

    if (!json_is_number(prob)) {
        WARN("probability not a number");
        return nullptr;
    }

    const float probability = json_number_value(prob) / 100.f;
    if (probability < 0 || probability > 1) {
        //SQWARN("bad probability %f", probability);
        return nullptr;
    }

    json_t* rhs = json_object_get(entryJson, _rhs);
    if (!rhs) {
        WARN("rule rhs not present");
        return nullptr;
    }
    if (!json_is_array(rhs)) {
        WARN("rhs is not an array");
        return nullptr;
    }
    std::vector<StochasticNote> productions;
    size_t index;
    json_t* prod;
    json_array_foreach(rhs, index, prod) {
        if (!json_is_string(prod)) {
            WARN("production not a string");
            return nullptr;
        }
        std::string s = json_string_value(prod);
        StochasticNote note = StochasticNote::fromString(s);
        if (!note.isValid()) {
            //SQWARN("bad note in rhs of grammar: %s", s.c_str());
        }
        productions.push_back(note);
    }

    auto entry = StochasticProductionRuleEntry::make();
    entry->probability = probability;
    entry->rhsProducedNotes = productions;
    assert(entry->isValid());
    return entry;
}

StochasticProductionRulePtr GMRSerialization::readRule(json_t* r) {
    //SQINFO("read rule called");
    assert(json_is_object(r));  // already checked.

    // get lhs string and make a rule from it
    json_t* lhs = json_object_get(r, _lhs);
    if (!lhs) {
        WARN("rule has no lhs");
        return nullptr;
    }
    if (!json_is_string(lhs)) {
        WARN("rule lhs not string");
        return nullptr;
    }
    std::string sLhs = json_string_value(lhs);

    StochasticNote lhsNote = StochasticNote::fromString(sLhs);
    if (!lhsNote.isValid()) {
        WARN("can't convert %s into note", sLhs.c_str());
        return nullptr;
    }

    StochasticProductionRulePtr ret = std::make_shared<StochasticProductionRule>(lhsNote);

    // remember one rule can have many productions - it's just for the lhs
    json_t* ents = json_object_get(r, _entries);

    size_t index;
    json_t* entry;
    json_array_foreach(ents, index, entry) {
        if (!json_is_object(entry)) {
            WARN("production entry not an object");
            return nullptr;
        }
        StochasticProductionRuleEntryPtr ruleEntry = readRuleEntry(entry);
        if (ruleEntry) {
            //SQINFO("adding entry to rule: ");
            ruleEntry->_dump();
            ret->addEntry(ruleEntry);
        }
    }

    return ret;
}

/*
StochasticGrammarPtr StochasticGrammar::getDemoGrammarSimple() {
    auto grammar = std::make_shared<StochasticGrammar>();

    auto rootRule = std::make_shared<StochasticProductionRule>(StochasticNote::half());
    assert(rootRule->isRuleValid());
    auto entry = StochasticProductionRuleEntry::make();
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->rhsProducedNotes.push_back(StochasticNote::quarter());
    entry->probability = .5;
    rootRule->addEntry(entry);
    assert(rootRule->isRuleValid());
    grammar->addRootRule(rootRule);
    return grammar;
}
*/

StochasticGrammarPtr GMRSerialization::readGrammar(json_t* theJson) {
    json_t* rules = json_object_get(theJson, _rules);
    if (!rules) {
        WARN("Json grammar has no rules");
        return nullptr;
    }
    if (!json_is_array(rules)) {
        WARN("rules is not an array");
        return nullptr;
    }

    StochasticGrammarPtr ret = std::make_shared<StochasticGrammar>();
    size_t index;
    json_t* rule;
    json_array_foreach(rules, index, rule) {
        if (!json_is_object(rule)) {
            WARN("rule is not an object");
            return nullptr;
        }
        StochasticProductionRulePtr r = readRule(rule);
        ret->addRule(r);
    }

    //SQINFO("read json grammar");
    if (ret->isValid()) {
        //SQINFO("using loaded grammar");
         ret->_dump();
        return ret;
    } else {
        //SQWARN("could not load grammer");
        return nullptr;
    }
}
