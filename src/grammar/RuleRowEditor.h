#pragma once

#include "../Squinky.hpp"
#include "../ctrl/SqUI.h"
#include "SqLog.h"

class StochasticProductionRuleEntry;
using StochasticProductionRuleEntryPtr = std::shared_ptr<StochasticProductionRuleEntry>;

class RuleRowEditor : public ModuleWidget {
public:
    RuleRowEditor(StochasticProductionRuleEntryPtr entry);
    ~RuleRowEditor() { //SQINFO("dtor or RuleRowEditor");
     }
    void draw(const DrawArgs& args) override;
    void step() override;

    //this is what we need to override to preveng UI bugs
    void onDragMove(const event::DragMove& e) override {
        //dump("overriddent on drag move");
        // eat onDragMove,or else some crazy logic will mess up our panel location.
        sq::consumeEvent(&e, this);
    }


private:
    // We don't own this module's lifetime - VCV will destroy it.
    Module* module = nullptr;
    StochasticProductionRuleEntryPtr entry;
    std::string ruleText;

    // void dump(const char* title) const;
};
