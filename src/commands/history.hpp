#pragma once
#include "../project/document.hpp"
#include <functional>

namespace trafficsim {
// One committed gesture is one named, atomic document change. Project has no command imports.
class History {
public:
    const ProjectDocument& document() const { return document_; }
    std::uint64_t revision() const { return document_.revision; }
    bool dirty() const { return revision() != saved_; }
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    struct State { std::uint64_t revision; std::string name; bool saved; };
    // Reachable states in chronological order, including the oldest retained snapshot.
    // Names are command keys; presentation/translation belongs to the shell.
    std::vector<State> states() const;
    std::string undoName() const { return canUndo() ? undo_.back().name : std::string{}; }
    std::string redoName() const { return canRedo() ? redo_.back().name : std::string{}; }
    // Navigate existing history without creating a revision or discarding redo states.
    // An unknown/expired revision throws before changing anything.
    bool restore(std::uint64_t revision);
    void reset(ProjectDocument document = {});
    void markSaved() { saved_ = revision(); }
    void markUnsaved() { saved_ = ~std::uint64_t{}; } // A recovered document requires an explicit save.
    bool execute(const std::string& name, const std::function<void(ProjectDocument&)>& change);
    void undo();
    void redo();
private:
    struct Entry { std::string name; ProjectDocument before, after;  };
    ProjectDocument document_;
    std::vector<Entry> undo_, redo_;
    std::uint64_t saved_{}, nextRevision_{1};
};
}
