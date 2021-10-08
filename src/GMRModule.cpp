
// This is the version from gmr3-clocks. should check on grm3 for changes

#include <sstream>

#include "Squinky.hpp"
#include "WidgetComposite.h"

#ifdef _GMR
#include "GMR2.h"
#include "ctrl/SqHelper.h"
#include "ctrl/SqMenuItem.h"
#include "ctrl/SqTooltips.h"
#include "ctrl/SqWidgets.h"
#include "grammar/GMRScreenHolder.h"
#include "grammar/GMRSerialization.h"
#include "grammar/GrammarRulePanel.h"

#include "seq/ClockFinder.h"

// #include "grammar/FakeScreen.h"
#include <osdialog.h>

using Comp = GMR2<WidgetComposite>;

/**
 */
struct GMRModule : Module {
public:
    GMRModule();
    /**
     *
     * Overrides of Module functions
     */
    //   void step() override;
    void process(const ProcessArgs& args) override;
    void onSampleRateChange() override;

    std::shared_ptr<Comp> comp;

    // Module "owns" the grammar;
    StochasticGrammarPtr grammar;

    void setNewGrammar(StochasticGrammarPtr gmr);

private:
};

void GMRModule::setNewGrammar(StochasticGrammarPtr gmr) {
    assert(gmr);
    grammar = gmr;
    comp->setGrammar(gmr);
}

void GMRModule::onSampleRateChange() {
    float rate = SqHelper::engineGetSampleRate();
    comp->setSampleRate(rate);
}

GMRModule::GMRModule() {
    config(Comp::NUM_PARAMS, Comp::NUM_INPUTS, Comp::NUM_OUTPUTS, Comp::NUM_LIGHTS);

    grammar = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::demo);
    comp = std::make_shared<Comp>(this);
    comp->setGrammar(grammar);

    std::shared_ptr<IComposite> icomp = Comp::getDescription();
    SqHelper::setupParams(icomp, this);

    onSampleRateChange();
    comp->init();
}

void GMRModule::process(const ProcessArgs& args) {
    comp->process(args);
}

////////////////////
// module widget
////////////////////

struct GMRWidget : ModuleWidget {
    GMRWidget(GMRModule*);

     void appendContextMenu(Menu* theMenu) override;
    void addLabel(const Vec& v, const char* str, const NVGcolor& color = SqHelper::COLOR_WHITE) {
        Label* label = new Label();
        label->box.pos = v;
        label->text = str;
        label->color = color;
        addChild(label);
    }

    //void test();
    void loadGrammarFile();
    void setNewGrammar(StochasticGrammarPtr);

    GMRScreenHolder* screenHolder = nullptr;
    GMRModule* theModule = nullptr;

    void addJacks(GMRModule* module);
    void addControls(GMRModule* module);

};

void GMRWidget::setNewGrammar(StochasticGrammarPtr gmr) {
    if (theModule) {
        theModule->setNewGrammar(gmr);
    }
    if (screenHolder) {
        screenHolder->setNewGrammar(gmr);
    }
}

void GMRWidget::appendContextMenu(Menu* theMenu) {
    MenuLabel* spacerLabel = new MenuLabel();
    theMenu->addChild(spacerLabel);

    ::rack::MenuItem* item = new SqMenuItem([]() { return false; }, [this]() {
            float rawClockValue = APP->engine->getParamValue(module, Comp::CLOCK_INPUT_PARAM);
            SeqClock2::ClockRate rate =  SeqClock2::ClockRate(int(std::round(rawClockValue)));
            const int div = SeqClock2::clockRate2Div(rate);
            ClockFinder::go(this, div, Comp::CLOCK_INPUT, Comp::RUN_INPUT, Comp::RESET_INPUT, ClockFinder::SquinkyType::GMR); });
    item->text = "Hookup Clock";
    theMenu->addChild(item);

    theMenu->addChild(new SqMenuItem(
        "Load grammar",
        []() {
            return false;  // we are never checked
        },
        [this]() {
            this->loadGrammarFile();
        }));
}

void GMRWidget::loadGrammarFile() {
    static const char FILTERS[] = "JSON grammar file (.json):json";
    osdialog_filters* filters = osdialog_filters_parse(FILTERS);
    std::string filename;

    // TODO: set a default?
    // std::string dir = _module->sequencer->context->settings()->getMidiFilePath();

    DEFER({
        osdialog_filters_free(filters);
    });

    char* pathC = osdialog_file(OSDIALOG_OPEN, nullptr, filename.c_str(), filters);

    if (!pathC) {
        // Fail silently
        return;
    }
    DEFER({
        std::free(pathC);
    });

    auto grammar = GMRSerialization::readGrammarFile(pathC);
    if (grammar) {
        setNewGrammar(grammar);
    }
}

/*
 CLOCK_INPUT,
        RESET_INPUT,
        RUN_INPUT,
    */
void GMRWidget::addJacks(GMRModule* module) {
    const float leftJack = 10;
    const float leftLabel = 4;
    const float jackY = 340;
    const float labelY = 320;
    const float dx = 40;

    addLabel(
        Vec(leftLabel - 4, labelY),
        "Clock");
    addInput(createInput<PJ301MPort>(
        Vec(leftJack, jackY),
        module,
        Comp::CLOCK_INPUT));

    addLabel(
        Vec(leftLabel + dx - 6, labelY),
        "Reset");
    addInput(createInput<PJ301MPort>(
        Vec(leftJack + dx, jackY),
        module,
        Comp::RESET_INPUT));

    addLabel(
        Vec(leftLabel + 2 * dx + 2, labelY),
        "Run");
    addInput(createInput<PJ301MPort>(
        Vec(leftJack + 2 * dx, jackY),
        module,
        Comp::RUN_INPUT));

    addLabel(
        Vec(leftLabel + 4 * dx + 2, labelY),
        "Out");
    addOutput(createOutput<PJ301MPort>(
        Vec(leftJack + 4 * dx, jackY),
        module,
        Comp::TRIGGER_OUTPUT));
}

void GMRWidget::addControls(GMRModule* module) {
    // let's add an invisible param so that clock finder will work
    std::shared_ptr<IComposite> icomp = Comp::getDescription();
#if 1
// try adding a hidden control 0, so clock finder can find by index.
    addParam(SqHelper::createParam<NullWidget>(
        icomp,
        Vec(7, 100),
        module, Comp::CLOCK_INPUT_PARAM));
#endif
        // add a hidden running control, just so ClockFinder can find it
    addParam(SqHelper::createParam<NullWidget>(
        icomp,
        Vec(0, 0),
        module,
        Comp::RUNNING_PARAM));
}

/**
 * Widget constructor will describe my implementation structure and
 * provide meta-data.
 * This is not shared by all modules in the DLL, just one
 */
GMRWidget::GMRWidget(GMRModule* module) {
    setModule(module);
    SqHelper::setPanel(this, "res/gmr_panel.svg");
    theModule = module;

    // make the active area almost full size, but don't cover up the jacks
    if (module) {
        assert(module->grammar);
        const Vec gmrPos = Vec(0, 0);
        const Vec gmrSize = Vec(this->box.size.x, 335);
        screenHolder = new GMRScreenHolder(gmrPos, gmrSize, module->grammar);
        addChild(screenHolder);
    }

    addJacks(module);
    addControls(module);
}

Model* modelGMRModule = createModel<GMRModule, GMRWidget>("squinkylabs-gmr");

#endif

#if 0       // gmr 3 version

#include <sstream>

#include "Squinky.hpp"
#include "WidgetComposite.h"

#ifdef _GMR
#include "GMR2.h"
#include "ctrl/SqHelper.h"
#include "ctrl/SqMenuItem.h"
#include "ctrl/SqTooltips.h"
#include "ctrl/SqWidgets.h"
#include "grammar/GMRScreenHolder.h"
#include "grammar/GMRSerialization.h"
#include "grammar/GrammarRulePanel.h"
// #include "grammar/FakeScreen.h"
#include <osdialog.h>

using Comp = GMR2<WidgetComposite>;

/**
 */
struct GMRModule : Module {
public:
    GMRModule();
    ~GMRModule() {
        //SQINFO("module dtor begin");
        comp.reset();
        grammar.reset();
        //SQINFO("module dtor end");
    }
    /**
     *
     * Overrides of Module functions
     */
    //   void step() override;
    void process(const ProcessArgs& args) override;
    void onSampleRateChange() override;

    std::shared_ptr<Comp> comp;

    // Module "owns" the grammar;
    StochasticGrammarPtr grammar;

    void setNewGrammar(StochasticGrammarPtr gmr);

private:
};

void GMRModule::setNewGrammar(StochasticGrammarPtr gmr) {
    assert(gmr);
    grammar = gmr;
    comp->setGrammar(gmr);
}

void GMRModule::onSampleRateChange() {
    float rate = SqHelper::engineGetSampleRate();
    comp->setSampleRate(rate);
}

GMRModule::GMRModule() {
    config(Comp::NUM_PARAMS, Comp::NUM_INPUTS, Comp::NUM_OUTPUTS, Comp::NUM_LIGHTS);

    grammar = StochasticGrammar::getDemoGrammar(StochasticGrammar::DemoGrammar::demo);
    comp = std::make_shared<Comp>(this);
    comp->setGrammar(grammar);

    std::shared_ptr<IComposite> icomp = Comp::getDescription();
    SqHelper::setupParams(icomp, this);

    onSampleRateChange();
    comp->init();
}

void GMRModule::process(const ProcessArgs& args) {
    //SQINFO(" GMRModule::process");
    comp->process(args);
}

////////////////////
// module widget
////////////////////

struct GMRWidget : ModuleWidget {
    GMRWidget(GMRModule*);
    ~GMRWidget() { //SQINFO("dtor of GMRWidget");
     }

    void appendContextMenu(Menu* theMenu) override;
    void addLabel(const Vec& v, const char* str, const NVGcolor& color = SqHelper::COLOR_WHITE) {
        Label* label = new Label();
        label->box.pos = v;
        label->text = str;
        label->color = color;
        addChild(label);
    }

    //void test();
    void loadGrammarFile();
    void setNewGrammar(StochasticGrammarPtr);

    GMRScreenHolder* screenHolder = nullptr;
    GMRModule* theModule = nullptr;
};

void GMRWidget::setNewGrammar(StochasticGrammarPtr gmr) {
    if (theModule) {
        theModule->setNewGrammar(gmr);
    }
    if (screenHolder) {
        screenHolder->setNewGrammar(gmr);
    }
}

void GMRWidget::appendContextMenu(Menu* theMenu) {
    MenuLabel* spacerLabel = new MenuLabel();
    theMenu->addChild(spacerLabel);
 
    theMenu->addChild(new SqMenuItem(
        "Load grammar",
        []() { 
            return false;  // we are never checked
        },
        [this]() {
            this->loadGrammarFile(); 
        }));
}

void GMRWidget::loadGrammarFile() {
    static const char FILTERS[] = "JSON grammar file (.json):json";
    osdialog_filters* filters = osdialog_filters_parse(FILTERS);
    std::string filename;

    // TODO: set a default?
    // std::string dir = _module->sequencer->context->settings()->getMidiFilePath();

    DEFER({
        osdialog_filters_free(filters);
    });

    char* pathC = osdialog_file(OSDIALOG_OPEN, nullptr, filename.c_str(), filters);

    if (!pathC) {
        // Fail silently
        return;
    }
    DEFER({
        std::free(pathC);
    });

    //SQINFO("load grammar from %s", pathC);
    auto grammar = GMRSerialization::readGrammarFile(pathC);
    if (grammar) {
        setNewGrammar(grammar);
    }
}

/**
 * Widget constructor will describe my implementation structure and
 * provide meta-data.
 * This is not shared by all modules in the DLL, just one
 */
GMRWidget::GMRWidget(GMRModule* module) {
    setModule(module);
    SqHelper::setPanel(this, "res/gmr_panel.svg");
    theModule = module;

    // make the active area almost full size, but don't cover up the jacks
    if (module) {
        assert(module->grammar);
        const Vec gmrPos = Vec(0, 0);
        const Vec gmrSize = Vec(this->box.size.x, 335);
        screenHolder = new GMRScreenHolder(gmrPos, gmrSize, module->grammar);
        addChild(screenHolder);
    }

    //SQINFO("adding the jacks at the bottom my h=%f", this->box.size.y);

    addLabel(
        Vec(30, 320),
        "Clock");
    addInput(createInput<PJ301MPort>(
        Vec(40, 340),
        module,
        Comp::CLOCK_INPUT));
    addLabel(
        Vec(80, 320),
        "Out");
    addOutput(createOutput<PJ301MPort>(
        Vec(90, 340),
        module,
        Comp::TRIGGER_OUTPUT));
}

Model* modelGMRModule = createModel<GMRModule, GMRWidget>("squinkylabs-gmr");

#endif
#endif


