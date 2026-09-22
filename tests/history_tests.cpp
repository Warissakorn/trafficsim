#include "test.hpp"
#include "../src/commands/appearance_commands.hpp"
#include "../src/commands/network_commands.hpp"
using namespace trafficsim;

TEST(history, navigate_saved_states_and_redo_without_new_revisions) {
    History h; h.reset();
    const auto empty = h.document();
    std::string id;
    h.execute("draw", [&](auto& d) { id=addLink(d,{{0,0},{100,0}},1,3.5); });
    h.markSaved(); const auto saved=h.document();
    h.execute("move", [&](auto& d) { translateObjects(d,{id},{3,2}); });
    const auto moved=h.document();
    h.execute("name", [&](auto& d) { renameObject(d,id,"Approach"); });
    const auto named=h.document();
    const auto states=h.states();
    CHECK(states.size()==4);
    CHECK(states[0].name.empty());
    CHECK(states[1].name=="draw" && states[1].saved);
    CHECK(states[2].name=="move" && !states[2].saved);
    CHECK(h.undoName()=="name" && h.redoName().empty());
    CHECK(h.restore(saved.revision));
    CHECK(h.document()==saved && !h.dirty());
    CHECK(h.undoName()=="draw" && h.redoName()=="move");
    CHECK(h.states().size()==4);
    CHECK(!h.restore(saved.revision));
    CHECK(h.restore(named.revision)); CHECK(h.document()==named);
    CHECK(h.restore(empty.revision)); CHECK(h.document()==empty);
    CHECK(h.restore(moved.revision)); CHECK(h.document()==moved);
    h.redo(); CHECK(h.document()==named);
}

TEST(history, failed_and_noop_edits_preserve_navigation_but_new_edits_branch) {
    History h; h.reset(); std::string id;
    h.execute("draw", [&](auto& d) { id=addLink(d,{{0,0},{100,0}},1,3.5); });
    h.execute("move", [&](auto& d) { translateObjects(d,{id},{1,0}); });
    const auto future=h.revision(); h.markSaved();
    h.undo(); const auto before=h.document();
    CHECK(!h.execute("noop", [](auto&) {}));
    test::throws([&] { h.execute("invalid", [&](auto& d) { changeLanes(d,id,{-1}); }); });
    test::throws([&] { h.restore(999999); }, "EDIT_HISTORY_REVISION");
    CHECK(h.document()==before && h.canRedo());
    CHECK(h.redoName()=="move" && h.states().back().saved);
    h.execute("branch", [&](auto& d) { renameObject(d,id,"Alternative"); });
    CHECK(!h.canRedo() && h.revision()>future);
    const auto branched=h.document();
    test::throws([&] { h.restore(future); }, "EDIT_HISTORY_REVISION");
    CHECK(h.document()==branched && h.dirty());
    for (const auto& state:h.states()) CHECK(!state.saved);
}

TEST(history, oldest_retained_state_is_reachable_after_eviction_and_reset) {
    History h; h.reset();
    for (int i=0;i<105;++i)
        h.execute("background", [i](auto& d) { d.background.x=i+1.; });
    const auto latest=h.document();
    const auto states=h.states();
    CHECK(states.size()==101 && states.front().revision==5);
    CHECK(h.restore(5)); CHECK(h.document().background.x==5);
    CHECK(!h.canUndo() && h.canRedo());
    test::throws([&] { h.restore(4); }, "EDIT_HISTORY_REVISION");
    CHECK(h.revision()==5);
    CHECK(h.restore(latest.revision)); CHECK(h.document()==latest);
    h.markUnsaved(); for (const auto& state:h.states()) CHECK(!state.saved);
    h.reset(latest);
    CHECK(h.states().size()==1 && h.states().front().saved && !h.dirty());
    CHECK(!h.canUndo() && !h.canRedo());
    test::throws([&] { h.restore(5); }, "EDIT_HISTORY_REVISION");
}
