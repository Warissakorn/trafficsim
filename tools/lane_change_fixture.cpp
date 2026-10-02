// Writes data/projects/lane-change-lab.traffic.json from tools/lane_change_network.hpp, its one
// source (D98). `lanelab` fails until the committed file matches, so the two cannot drift.
//
//   trafficsim-lane-change-fixture <output.traffic.json>
#include "lane_change_network.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
using namespace trafficsim;
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "usage: trafficsim-lane-change-fixture <output.traffic.json>\n"; return 2; }
    std::ofstream out(argv[1], std::ios::binary);
    out << documentJson(fixture::laneChangeLab().document).dump(2) << "\n";
    if (!out) { std::cerr << "could not write " << argv[1] << "\n"; return 1; }
    return 0;
}
