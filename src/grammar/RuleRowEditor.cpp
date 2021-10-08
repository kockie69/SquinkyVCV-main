
#include "RuleRowEditor.h"

#include "../ctrl/SqWidgets.h"
#include "../seq/SqGfx.h"
#include "GMR2.h"
#include "StochasticProductionRule.h"

using Comp = GMR2<WidgetComposite>;

RuleRowEditor::RuleRowEditor(StochasticProductionRuleEntryPtr e) : entry(e) {
    module = new Module();
    this->setModule(module);
    module->config(1, 0, 0, 0);

    //SQINFO("ctol of rule row editor");

    const int paramId = 0;
    const float prob = entry->probability;

    module->configParam(paramId, 0, 100, prob * 100, "probability");

    auto p = new RoganSLBlue30();
    p->box.pos.x = 150;
    p->box.pos.y = 4;
    //p->getParamQuantity() = module->paramQuantities[paramId];
    addParam(p);

    if (entry->rhsProducedNotes.size() == 1) {
        ruleText = entry->rhsProducedNotes[0].toText();  // is this possible?
    } else {
        for (auto note : entry->rhsProducedNotes) {
            ruleText += note.toText();
            ruleText += ", ";
        }
        ruleText.pop_back();        // remove the trailing comma
        ruleText.pop_back();        // and space
    }
}

void RuleRowEditor::draw(const DrawArgs &args) {
    auto vg = args.vg;

    //NVGcolor pale = nvgRGBAf(1, 0, 0, .2);
    //SqGfx::filledRect(vg, pale, 0, 0, this->box.size.x, this->box.size.y);

    SqGfx::drawText2(vg, 10, 20, ruleText.c_str(), 12, SqHelper::COLOR_WHITE);
    OpaqueWidget::draw(args);
}

void RuleRowEditor::step() {
    const int value = int(std::round(APP->engine->getParamValue(module, 0)));
    if (value != int(entry->probability * 100)) {
        //SQINFO("knob change! knob value=%d, existing probx100 = %d this=%p", value, int(entry->probability * 100), this);
        entry->probability = 0.01 * value;
    }
    ModuleWidget::step();
}