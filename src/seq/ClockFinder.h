#pragma once

namespace rack {
namespace ui {
struct Menu;
}
namespace app {
struct ModuleWidget;
}
}  // namespace rack

using Menu = ::rack::ui::Menu;
using ModuleWidget = ::rack::app::ModuleWidget;

class ClockFinder {
public:
    enum class SquinkyType {
        SEQPP,
        X4X,
        GMR
    };
    static void go(ModuleWidget* self, int clockDivSetting, int clockInput, int runInput, int resetInput, SquinkyType module);
};