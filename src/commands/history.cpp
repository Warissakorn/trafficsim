#include "history.hpp"
#include <limits>

namespace trafficsim {
void History::reset(ProjectDocument document) {
    validateDocument(document);
    document_ = std::move(document); undo_.clear(); redo_.clear(); saved_ = revision(); nextRevision_ = revision() + 1;
}
bool History::execute(const std::string& name, const std::function<void(ProjectDocument&)>& change) {
    auto candidate = document_;
    change(candidate);
    validateDocument(candidate); // Failed commands leave model, history, saved state untouched.
    // Value comparison, not serialisation: documentJson-ing both documents on every edit cost
    // more than the command and its validation together on a large network.
    if (candidate == document_) return false;
    // Only the revision changed since validateDocument above, and the sole thing it checks
    // about a revision is that it has not run out; re-validating the whole document again
    // would recompile the scenario for that one test.
    if (nextRevision_ == std::numeric_limits<std::uint64_t>::max()) throw std::invalid_argument("EDIT_ID_LIMIT");
    candidate.revision = nextRevision_;
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
