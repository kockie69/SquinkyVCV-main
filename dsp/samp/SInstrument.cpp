
#include "SInstrument.h"

#include "SqLog.h"

void SInstrument::_dump() {
    const int hsize = headings.size();
    //SQINFO("Num Headings = %d", hsize);
    for (auto h : headings) {
        //SQINFO("--h:");
        //SHeading::dumpKeysAndValues(headings);
        h->_dump();
    }
}
