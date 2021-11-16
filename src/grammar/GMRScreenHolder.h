#pragma once
#include "../Squinky.hpp"
#include "SqLog.h"


class GMRTabbedHeader;
class StochasticGrammar;
using StochasticGrammarPtr = std::shared_ptr<StochasticGrammar>;

class GMRScreenHolder : public OpaqueWidget {
public:
    GMRScreenHolder(const Vec &pos, const Vec &size, StochasticGrammarPtr);
    ~GMRScreenHolder();
    void draw(const DrawArgs &args) override;

    void setNewGrammar(StochasticGrammarPtr gmr);
private:
    StochasticGrammarPtr grammar;
    GMRTabbedHeader* header = nullptr;
    /**
     * these are the screens we are managing.
     * at any one time one of them will be a child of us
     */
    std::vector<Widget*> screens; 
    void onNewTab(int index);
    int currentTab=0; 

    void sizeChild(Widget*);         
};
