// Writes data/projects/t-junction-priority.traffic.json from tools/t_junction_network.hpp, its one
// source (M3.2.7). `tjunction` fails until the committed file matches, so the two cannot drift.
//
//   trafficsim-t-junction-fixture <output.traffic.json>
#include "t_junction_network.hpp"
#include <fstream>
#include <iostream>
using namespace trafficsim;
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: trafficsim-t-junction-fixture <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[1], std::ios::binary);
    out << documentJson(fixture::tJunction().document).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[1] << "\n"; return 1; }
    return 0;
}
