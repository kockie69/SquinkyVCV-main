
#include "StochasticNote.h"

#include "SqLog.h"
#include <assert.h>

// TODO: make better
const static int _ppq = 96;
const int StochasticNote::ppq = _ppq;

bool StochasticNote::operator<(const StochasticNote& other) const {
    return other.duration < this->duration;
}

static const int half_dur = _ppq * 2;
static const int quarter_dur = _ppq;
static const int eighth_dur = _ppq / 2;
static const int sixteenth_dur = eighth_dur / 2;
static const int thirtysecond_dur = sixteenth_dur / 2;

StochasticNote StochasticNote::half() {
    return StochasticNote(half_dur);
}

StochasticNote StochasticNote::quarter() {
    return StochasticNote(quarter_dur);
}

StochasticNote StochasticNote::eighth() {
    return StochasticNote(eighth_dur);
}

StochasticNote StochasticNote::sixteenth() {
    return StochasticNote(sixteenth_dur);
}

StochasticNote StochasticNote::thirtysecond() {
    return StochasticNote(thirtysecond_dur);
}


StochasticNote StochasticNote::dotted_half() {
    return StochasticNote(half_dur * 3 / 2);
}

StochasticNote StochasticNote::dotted_quarter() {
    return StochasticNote(quarter_dur * 3 / 2);
}

StochasticNote StochasticNote::dotted_eighth() {
    return StochasticNote(eighth_dur * 3 / 2);
}

StochasticNote StochasticNote::dotted_sixteenth() {
    return StochasticNote(sixteenth_dur * 3 / 2);
}

StochasticNote StochasticNote::trip_half() {
    return StochasticNote(half_dur * 2 / 3);
}

StochasticNote StochasticNote::trip_quarter() {
    return StochasticNote(quarter_dur * 2 / 3);
}

StochasticNote StochasticNote::trip_eighth() {
    return StochasticNote(eighth_dur * 2 / 3);
}

StochasticNote StochasticNote::trip_sixteenth() {
    return StochasticNote(sixteenth_dur * 2 / 3);
}

StochasticNote StochasticNote::fromString(const std::string& string) {
    if (string == "h")
        return half();
    if (string == "q")
        return quarter();
    if (string == "e")
        return eighth();
    if (string == "x" || string == "s")
        return sixteenth();
     if (string == ".h")
        return dotted_half();
    if (string == ".q")
        return dotted_quarter();
    if (string == ".e")
        return dotted_eighth();
    if (string == ".x" || string == ".s")
        return dotted_sixteenth();
    if (string == "3q")
        return trip_quarter();
    if (string == "3h")
        return trip_half();
     if (string == "3e")
        return trip_eighth();
     if (string == "3s" || string == "3x")
        return trip_sixteenth();
#if 0
    if (string == "s")
        return sixteenth();
    if (string == "x")
        return thirtysecond();
#endif
    return StochasticNote(0);
}

std::string StochasticNote::toText() const {
    std::string ret;
    switch (duration) {
        case half_dur:
            ret = "1/2";
            break;
        case quarter_dur:
            ret = "1/4";
            break;
        case eighth_dur:
            ret = "1/8";
            break;
        case sixteenth_dur:
            ret = "1/16";
            break;
        case half_dur * 3 / 2:
            ret = ".-1/2";
            break;
        case quarter_dur * 3 / 2:
            ret = ".-1/4";
            break;
        case eighth_dur * 3 / 2:
            ret = ".-1/8";
            break;
        case sixteenth_dur * 3 / 2:
            ret = ".-1/16";
            break;
        case half_dur * 2 / 3:
            ret = "1/2(3)";
            break;
        case quarter_dur * 2 / 3:
            ret = "1/4(3)";
            break;
        case eighth_dur * 2 / 3:
            ret = "1/8(3)";
            break;
        case sixteenth_dur * 2 / 3:
            ret = "1/16(3)";
            break;
        default:
            //SQWARN("no case for %d", duration);
            assert(false);
    }
    return ret;
}