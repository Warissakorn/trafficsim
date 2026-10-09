## 2026-10-06 — Routes recognized at clicked Link stations (D119)

Owner authorized choosing a destination when vehicles reach the first Route click.
An optional RoutingDecision.position stores a reference-polyline station; positioned
documents opt into schema 20, while absent positions retain legacy demand-time booking.
The Route gesture creates/reuses the station atomically with its traced Route. A shared
crossbar previews and selects the point; dragging commits one Undo step. The dialog can
opt existing decisions in. Link inputs use the decision; explicitly assigned Route inputs
retain their assignment. Pending demand preview destinations are labelled as deferred.

Neutral provisional route families preserve source volume and lane shares, retain all
period/type alternatives and distinguish different traces to the same end Link. At passage,
the core draws once using seeded RNG and passage-time half-open interval weights, records
the decision on the vehicle and emits a routing event. Compatible physical prefixes retain
distance and lane; generated lateral awareness starts at the station. Legacy downstream
bookings remain intact. Decisions in active conflict reservation spans are refused.

Recognition caps the crossing tick's displacement at the line and retains computed speed;
the selected suffix receives normal safety checks on the next tick. Excess proposed travel
is discarded, so recognition time is quantized by dt; whole-trip timing impact is unmeasured.
The initial slice retains one decision per Link and excludes Connector stations. The
contract and independent M2.1.3 platform/owner gates document these limits. Oldest complete
D105/D106 progress entries moved to the indexed archive to keep this file near 500 lines.

Validation: GCC 13.3/C++20 and Qt 6.4.2 desktop build on Linux. All **91/91 CTest groups**
pass after updating the existing future-schema rejection example from 20 to 999 and
rerunning that affected group. New core/compiler coverage includes nine station cases;
Qt offscreen coverage verifies gesture, overlay clipping, grouping, drag/cancel, Undo and
save/reopen. Frozen references and architecture/file-size/documentation guards pass.
Native Linux/Windows CI and owner appearance/fidelity remain separate gates.
