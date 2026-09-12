#include "history.hpp"

namespace trafficsim {
void History::reset(ProjectDocument document) {
    validateDocument(document);
    document_ = std::move(document); undo_.clear(); redo_.clear(); saved_ = revision(); nextRevision_ = revision() + 1;
}
bool History::execute(const std::string& name, const std::function<void(ProjectDocument&)>& change) {
    auto candidate = document_;
    change(candidate);
    validateDocument(candidate); // Failed commands leave model, history, saved state untouched.
    if (documentJson(candidate) == documentJson(document_)) return false;
    candidate.revision = nextRevision_;
    validateDocument(candidate);
    ++nextRevision_;
    Entry entry{name, document_, std::move(candidate)};
    document_ = entry.after;
    undo_.push_back(std::move(entry)); redo_.clear();
    if (undo_.size() > 100) undo_.erase(undo_.begin());
    return true;
}
void History::undo() {
    if (!canUndo()) return;
    auto entry = std::move(undo_.back()); undo_.pop_back();
    document_ = entry.before; redo_.push_back(std::move(entry));
}
void History::redo() {
    if (!canRedo()) return;
    auto entry = std::move(redo_.back()); redo_.pop_back();
    document_ = entry.after; undo_.push_back(std::move(entry));
}
}
