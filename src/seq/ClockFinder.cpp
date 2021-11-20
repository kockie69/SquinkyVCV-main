#include "WidgetComposite.h"
#include "ClockFinder.h"

#include "GMR2.h"
#include "Seq.h"
#include "Seq4.h"

using ModuleWidget = ::rack::app::ModuleWidget;
using Model = ::rack::plugin::Model;
using PortWidget = ::rack::app::PortWidget;
using CableWidget = ::rack::app::CableWidget;
using ParamWidget = ::rack::app::ParamWidget;
using Engine = ::rack::engine::Engine;

static float getParamValueFromWidget(ModuleWidget* moduleWidget, ParamWidget* paramWidget) {
    auto module = moduleWidget->module;
    const int paramId = paramWidget->getParamQuantity()->paramId;
    float ret = APP->engine->getParamValue(module, paramId);
    return ret;
}

/********************************* ALL STUFF ABOUT EXTERNAL CLOCKS **************/

/**
 * Descriptions of all the clocks we know about
 */
class ClockDescriptor {
public:
    std::string slug;
    int clockOutputIds[3];
    int clockRatioParamIds[3];  // for the three stand-alone, not counting master
    int runningParamId;
    bool runningIsInverted;
    //void dump() const;

    ClockDescriptor(const ClockDescriptor&) = delete;
    const ClockDescriptor& operator=(const ClockDescriptor&) = delete;
};

static const ClockDescriptor descriptors[] = {
    {"Clocked",
     {1, 2, 3},
     {1, 2, 3},
     13,
     true},
    {"Clocked-Clkd",
     {1, 2, 3},
     {0, 1, 2},
     5,
     true}};

#if 0
void ClockDescriptor::dump() const
{
    assert((this == descriptors + 0) || (this == descriptors + 1)); 
    INFO("**** dump descriptor %p", this);
    INFO("    slug = %s", slug.c_str());
    INFO("    output ids = %d,%d,%d", clockOutputIds[0],clockOutputIds[1],clockOutputIds[2]);
    INFO(" runningParamID = %d", runningParamId);
}

static void dumpBoth()
{
    INFO("dump both");
    descriptors[0].dump();
    descriptors[1].dump();
    INFO("dump both done");

}
#endif

class Seqs {
public:
    static std::vector<PortWidget*> findInputs(ModuleWidget* seq);
    static bool anyConnected(const std::vector<PortWidget*>& ports);
    static float clockDivToClockedParam(int div);
    static ParamWidget* getRunningParam(ModuleWidget*, ClockFinder::SquinkyType);
};

static const int numDescriptors = sizeof(descriptors) / sizeof(ClockDescriptor);

// class that knows about clocks from other manufacturers
class Clocks {
public:
    using WidgetAndDescription = std::pair<ModuleWidget*, const ClockDescriptor*>;

    static WidgetAndDescription findClosestClocked(const ModuleWidget* fromSeq);

    /**
     * return.first will be a non null port widget to use for clock/
     * return.second will be true if the port is already patched
     */
    static std::pair<PortWidget*, bool> findBestClockOutput(WidgetAndDescription clocked, int div);

    static std::vector<PortWidget*> findClockedOutputs(ModuleWidget* clocked, PortWidget* clock);

    static ParamWidget* getRatioParam(WidgetAndDescription clocked, int index);
    static ParamWidget* getRunningParam(WidgetAndDescription clocked);
    static float getRunningLightValue(WidgetAndDescription clocked);

private:
    using WidgetAndDescriptionS = std::vector<WidgetAndDescription>;
    static WidgetAndDescriptionS findClocks();

    /**
     * get the port for clock[index] (skipping over "master" clock)
     */
    static PortWidget* findClockOutput(WidgetAndDescription clock, int index);
};

Clocks::WidgetAndDescriptionS Clocks::findClocks() {
    WidgetAndDescriptionS ret;
    auto rack = APP->scene->rack;
    for (::rack::widget::Widget* w2 : rack->getModuleContainer()->children) {
        ModuleWidget* modwid = dynamic_cast<ModuleWidget*>(w2);
        if (modwid) {
            Model* model = modwid->model;
            for (int i = 0; i < numDescriptors; ++i) {
                const ClockDescriptor* descriptor = descriptors + i;
                if (model->slug == descriptor->slug) {
                    ret.push_back(std::make_pair(modwid, descriptor));
                }
            }
        } else {
            WARN("was not a module widget");
        }
    }
    return ret;
}

static double calcDistance(const ModuleWidget* a, const ModuleWidget* b) {
    assert(a && b);
    auto rect = a->box.expand(b->box);
    const auto x = rect.size.x;
    const auto y = rect.size.y;
    return std::sqrt(x * x + y * y);
}

Clocks::WidgetAndDescription Clocks::findClosestClocked(const ModuleWidget* from) {
    assert(from);
    WidgetAndDescriptionS clocks = findClocks();
    WidgetAndDescription ret(nullptr, nullptr);
    double closestDistance = 1000000000000000;
    for (auto clock : clocks) {
        double distance = calcDistance(clock.first, from);
        if (distance < closestDistance) {
            closestDistance = distance;
            ret = clock;
        }
    }
    return ret;
}

/**
 * return.first will be a non null port widget to use for clock/
 * return.second will be true if the port is already patched.
 * 
 * will try to find an unpatched port
 */
std::pair<PortWidget*, bool> Clocks::findBestClockOutput(WidgetAndDescription clock, int div) {
    for (int i = 0; i < 3; ++i) {
        auto port = findClockOutput(clock, i);
        auto cables = APP->scene->rack->getCablesOnPort(port);
        if (cables.empty()) {
            return std::make_pair(port, false);
        }

        // if it's patched, look at the mult. If it's a match, we can use it, too
        auto paramWidget = getRatioParam(clock, i);
        if (paramWidget) {
            const float targetValue = Seqs::clockDivToClockedParam(div);
            const float actualValue = getParamValueFromWidget(clock.first, paramWidget);

            if (int(std::round(targetValue)) == int(std::round(actualValue))) {
                return std::make_pair(port, true);
            }
        }
    }
    return std::make_pair(findClockOutput(clock, 0), true);
}

PortWidget* Clocks::findClockOutput(WidgetAndDescription clock, int index) {
    const int targetPortId = clock.second->clockOutputIds[index];
    for (auto output : clock.first->getOutputs()) {
        if ((output->portId) == targetPortId) {
            return output;
        }
    }
    assert(false);
    return nullptr;
}

// TODO: this uses hard coded output port numbers
std::vector<PortWidget*> Clocks::findClockedOutputs(ModuleWidget* clocked, PortWidget* clock) {
    int found = 1;
    std::vector<PortWidget*> ret(3);

    assert(clock);
    ret[0] = clock;
    for (auto output : clocked->getOutputs()) {
        switch (output->portId) {
            case 5:  // run
                ret[1] = output;
                ++found;
                break;
            case 4:  // reset
                ret[2] = output;
                ++found;
                break;
        }
    }
    return (found == 3) ? ret : std::vector<PortWidget*>();
}

/**
 * Given a Clocked module and an index into one of the non-master clocks,
 * Find it and return the param widget for the ratio knob.
 */
ParamWidget* Clocks::getRatioParam(WidgetAndDescription clocked, int _index) {
    const int clockRatioParamId = clocked.second->clockRatioParamIds[_index];
    for (auto param : clocked.first->getParams()) {
        if (!param->getParamQuantity()) {
            WARN("param has no quantity");
            return nullptr;
        }
        const int id = param->getParamQuantity()->paramId;
        if (id == clockRatioParamId) {
            return param;
        }
    }
    assert(false);
    return nullptr;
}

static ParamWidget* findParamWidgetForParamId(ModuleWidget* moduleWidget, int paramID) {
    assert(paramID >= 0);
    assert(paramID < 20);
    for (auto param : moduleWidget->getParams()) {
        if (!param->getParamQuantity()) {
            WARN("param has no quantity");
            return nullptr;
        }
        const int id = param->getParamQuantity()->paramId;
        if (id == paramID) {
            return param;
        }
    }

    assert(false);
    return nullptr;
}

// really should return Light*
static Light findLightForId(ModuleWidget* moduleWidget, int lightID) {
    assert(lightID >= 0);
    assert(lightID < 20);
    Module* module = moduleWidget->module;

    Light light = module->lights[lightID];
    return light;
}

float Clocks::getRunningLightValue(WidgetAndDescription clocked) {
    const int lightID = 1;  // from clkd source code
    Light light = findLightForId(clocked.first, lightID);
    return light.value;
}

ParamWidget* Clocks::getRunningParam(WidgetAndDescription clocked) {
    const int paramID = clocked.second->runningParamId;
    return findParamWidgetForParamId(clocked.first, paramID);
}

ParamWidget* Seqs::getRunningParam(ModuleWidget* seqWidget, ClockFinder::SquinkyType moduleType) {
    //  const int paramID = isSeqPlusPlus ?  int(Seq<WidgetComposite>::RUNNING_PARAM) : int(Seq4<WidgetComposite>::RUNNING_PARAM);
    int paramID = 0;
    switch (moduleType) {
        case ClockFinder::SquinkyType::SEQPP:
            paramID = int(Seq<WidgetComposite>::RUNNING_PARAM);
            break;
        case ClockFinder::SquinkyType::X4X:
            paramID = int(Seq4<WidgetComposite>::RUNNING_PARAM);
            break;
        case ClockFinder::SquinkyType::GMR:
            paramID = int(GMR2<WidgetComposite>::RUNNING_PARAM);
            break;
        default:
            assert(false);
    }
    return findParamWidgetForParamId(seqWidget, paramID);
}

// TODO: generalize to both seq
// clock, run, reset

std::vector<PortWidget*> Seqs::findInputs(ModuleWidget* seq) {
    int found = 0;
    std::vector<rack::app::PortWidget*> ret(3);

    for (rack::app::PortWidget* input : seq->getInputs()) {
        switch (input->portId) {
            case 0:
                ret[0] = input;
                ++found;
                break;
            case 1:
                ret[2] = input;
                ++found;
                break;
            case 2:
                ret[1] = input;
                ++found;
                break;
        }
    }
    return (found == 3) ? ret : std::vector<PortWidget*>();
}

bool Seqs::anyConnected(const std::vector<PortWidget*>& ports) {
    for (auto port : ports) {
        auto cables = APP->scene->rack->getCablesOnPort(port);
        if (!cables.empty()) {
            return true;
        }
    }
    return false;
}

// TODO: get this from descriptor - don't assume ratios
float Seqs::clockDivToClockedParam(int div) {
    float ret = 0;
    assert(div > 0);
    if (div <= 16) {
        ret = div + 1;
    } else if (div == 32) {
        ret = 7 + 17;
    } else if (div == 64) {
        ret = 7 + 17 + 9;
    } else if (div == 96) {
        ret = 7 + 17 + 9 + 1;
    } else {
        assert(false);
    }
    return ret;
}

static void setParamValueOnWidget(ModuleWidget* moduleWidget, ParamWidget* paramWidget, float value) {
    auto module = moduleWidget->module;
    const int paramId = paramWidget->getParamQuantity()->paramId;
    APP->engine->setParamValue(module, paramId, value);
}

void ClockFinder::go(ModuleWidget* host, int div, int clockInput, int runInput, int resetInput, SquinkyType moduleType) {
    // const bool isSeqPlusPlus = true;

    // we are hard coded to these values. If module changes, we will have to get smarter/
    assert(clockInput == 0);
    assert(runInput == 2);
    assert(resetInput == 1);

    auto moduleAndDescription = Clocks::findClosestClocked(host);
    if (!moduleAndDescription.first) {
        return;
    }

    auto clockOutput = Clocks::findBestClockOutput(moduleAndDescription, div);
    assert(clockOutput.first != nullptr);

    auto outputs = Clocks::findClockedOutputs(moduleAndDescription.first, clockOutput.first);
    assert(outputs.size() == 3);

    auto inputs = Seqs::findInputs(host);
    assert(inputs.size() == 3);

    if (outputs.size() != 3 || inputs.size() != 3) {
        WARN("bad I/O matchup. o=%lu, i=%lu", (unsigned long)outputs.size(), (unsigned long)inputs.size());
        return;
    }

    if (Seqs::anyConnected(inputs)) {
        return;
    }

    // const NVGcolor color = nvgRGB(0, 0, 0);
    for (int i = 0; i < 3; ++i) {
        rack::engine::Cable* cable = new rack::engine::Cable;
        cable->inputModule=host->getModule();
        cable->outputModule=moduleAndDescription.first->getModule();
        cable->id=-1;      
        for (PortWidget *port : outputs) {
            if (port->portId == outputs[i]->portId) {
                cable->outputId=port->portId;
                break;
            }
        }
        for (PortWidget *port : inputs) {
            if (port->portId == inputs[i]->portId) {
                cable->inputId=port->portId;
                break;
            }
        }
        APP->engine->addCable(cable);
        rack::app::CableWidget* cw = new rack::app::CableWidget;
	    cw->setCable(cable);
        cw->color = APP->scene->rack->getNextCableColor();
	    if (cw->isComplete())
            APP->scene->rack->addCable(cw);
    }
    
    if (!clockOutput.second) {
        // if the clock output was empty before us, then we can set the
        // ratio to match the sequencer.
        const int clockIndex = clockOutput.first->portId - 1;
        auto paramWidget = Clocks::getRatioParam(moduleAndDescription, clockIndex);
        if (paramWidget) {
            const float value = Seqs::clockDivToClockedParam(div);
            setParamValueOnWidget(moduleAndDescription.first, paramWidget, value);
        }
    }

    // now make Seq agree with clock re: running
    {
        const float runLight = Clocks::getRunningLightValue(moduleAndDescription);
        auto seqParamWidget = Seqs::getRunningParam(host, moduleType);
        assert(seqParamWidget);
        if (seqParamWidget) {
            bool clockIsRunning = runLight > .5;
            setParamValueOnWidget(host, seqParamWidget, clockIsRunning ? 1 : 0);
        }
    }
}
