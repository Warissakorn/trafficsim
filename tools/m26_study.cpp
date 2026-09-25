// Writes data/projects/m2.6-study-template.traffic.json from tools/m26_study_network.hpp, its one
// source. `m26study` fails until the committed file matches, so the two cannot drift.
//
//   trafficsim-m26-study <output.traffic.json>
#include "m26_study_network.hpp"
#include <fstream>
#include <iostream>
using namespace trafficsim;
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: trafficsim-m26-study <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[1], std::ios::binary);
    out << documentJson(fixture::m26StudyTemplate()).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[1] << "\n"; return 1; }
    return 0;
}
