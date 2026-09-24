// Writes data/projects/four-leg-signalised.traffic.json from tools/four_leg_network.hpp, the
// drawing's one source. Run it after changing the builder; `four-leg` fails until the committed
// file matches, so the two cannot drift.
//
//   trafficsim-four-leg-fixture <output.traffic.json>
#include "four_leg_network.hpp"
#include <fstream>
#include <iostream>
using namespace trafficsim;
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: trafficsim-four-leg-fixture <output.traffic.json>\n"; return 2; }
    const auto built = fixture::fourLegIntersection();
    std::ofstream out(argv[1], std::ios::binary);
    // The editor's own serialisation (src/shell/editor_storage.cpp), so the file reads as saved.
    out << documentJson(built.document).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[1] << "\n"; return 1; }
    return 0;
}
