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
    void reset(ProjectDocument document = {});
    void markSaved() { saved_ = revision(); }
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
