#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "math.hpp"
class InputScreen;
class MidiSequencer;
namespace rack {
namespace widget {
struct Widget;
}
}  // namespace rack

using InputScreenPtr = std::shared_ptr<InputScreen>;
using MidiSequencerPtr = std::shared_ptr<MidiSequencer>;

class InputScreenManager {
public:
    enum class Screens { Invert,
                         Transpose,
                         ReversePitch,
                         ChopNotes,
                         QuantizePitch,
                         MakeTriads };
    using Callback = std::function<void()>;

    InputScreenManager(::rack::math::Vec size);
    /**
     * If you call it while a screen is up the call will be ignored.
     */
    // TODO: get rid of callback
    void show(::rack::widget::Widget* parent, Screens, MidiSequencerPtr, Callback);

    /**
     * If you destroy the manager while a screen is up,
     * it's ok. Everything will be cleaned up
     */
    ~InputScreenManager();

    static const char* xformName(Screens);

private:
    ::rack::widget::Widget* parentWidget = nullptr;
    const ::rack::math::Vec size;
    InputScreenPtr screen;
    ::rack::widget::Widget* parent = nullptr;
    Callback callback = nullptr;

    void dismiss(bool bOK);

    template <class T>
    std::shared_ptr<T> make(
        const ::rack::math::Vec& size,
        MidiSequencerPtr seq,
        std::function<void(bool)> dismisser);
};