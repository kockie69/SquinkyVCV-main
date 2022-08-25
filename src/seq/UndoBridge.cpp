
#include "../Squinky.hpp"
#include "SqCommand.h"
#include "UndoRedoStack.h"

// std C
#include <assert.h>

#if defined(__USE_VCV_UNDO) && defined(_SEQ)

#include "../SequencerModule.h"
#include "../Sequencer4Module.h"

template<class SequencerPtr, class Command, class Module, class Widget>
class SeqAction : public ::rack::history::ModuleAction
{
public:
    SeqAction(const std::string& _name, std::shared_ptr<Command> command, int64_t moduleId, const std::string& moduleName)
    {
        wrappedCommand = command;
        this->name = moduleName + ": " + wrappedCommand->name;
        this->moduleId = moduleId;
    }
    void undo() override
    {
        SequencerPtr seq = getSeq();
        Widget* wid = getWidget();
        if (seq && wid) {
            wrappedCommand->undo(seq, wid);
        }
    }
    void redo() override
    {
        SequencerPtr seq = getSeq();
        Widget* wid = getWidget();
        if (seq && wid) {
            wrappedCommand->execute(seq, wid);
        }
    }

private:
    std::shared_ptr<Command> wrappedCommand;
    SequencerPtr getSeq()
    {
        SequencerPtr ret;
        Module* module = dynamic_cast<Module *>(APP->engine->getModule(moduleId));
        if (!module) {
            WARN("error getting module in undo, id = %I64d", moduleId);
            return ret;
        }
        ret = module->getSequencer();
        if (!ret) {
            WARN("error getting sequencer in undo");
        }
        return ret;
    }
    Widget* getWidget()
    {
        Module* module = dynamic_cast<Module *>(APP->engine->getModule(moduleId));
        if (!module) {
            WARN("error getting module in undo, id = %I64d", moduleId);
            return nullptr;
        }
        Widget* widget = module->widget;
        if (!widget) {
            WARN("error getting widget in undo");
            return nullptr;
        }
        return widget;
    }
};

using SeqAction1 = SeqAction<MidiSequencerPtr, SqCommand, SequencerModule, SequencerWidget>;
#ifdef _SEQ4
using SeqAction4 = SeqAction<MidiSequencer4Ptr, Sq4Command, Sequencer4Module, Sequencer4Widget>;
#endif

void UndoRedoStack::setModuleId(int64_t id, bool fromUI)
{   
    if (id != this->moduleId) {
         this->moduleId = id;
    }
}

#ifdef _SEQ4
void UndoRedoStack::execute4(MidiSequencer4Ptr seq, Sequencer4Widget* widget, std::shared_ptr<Sq4Command> cmd)
{
    assert(seq);
    cmd->execute(seq, widget);
    auto action = new SeqAction4("unknown", cmd, moduleId, "4X4");

    APP->history->push(action);
}

void UndoRedoStack::execute4(MidiSequencer4Ptr seq, std::shared_ptr<Sq4Command> cmd)
{
    assert(seq);
    cmd->execute(seq, nullptr);
    auto action = new SeqAction4("unknown", cmd, moduleId, "4X4");

    APP->history->push(action);
}
#endif

void UndoRedoStack::execute(MidiSequencerPtr seq, SequencerWidget* widget, std::shared_ptr<SqCommand> cmd)
{
    assert(seq);
    cmd->execute(seq, widget);
    auto action = new SeqAction1("unknown", cmd, moduleId, "Seq++");

    APP->history->push(action);
}
void UndoRedoStack::execute(MidiSequencerPtr seq, std::shared_ptr<SqCommand> cmd)
{
    assert(seq);
    cmd->execute(seq, nullptr);
    auto action = new SeqAction1("unknown", cmd, moduleId, "Seq++");

    APP->history->push(action);
}

#endif

#if defined(__USE_VCV_UNDO) && !defined(_SEQ)

void UndoRedoStack::setModuleId(uint64_t id)
{
    ;
}

void UndoRedoStack::execute(MidiSequencerPtr seq, std::shared_ptr<SqCommand> cmd)
{

}
#endif
