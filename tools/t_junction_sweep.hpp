#pragma once
// M3.2.7b: the diagnostic seeded sweep of docs/M3_ACCEPTANCE.md §2 over the T-junction fixture.
// Its inputs are fixed here and committed as metadata BEFORE any output is observed; `tjunction`
// checks the committed metadata still describes the fixture, so an archived sweep cannot silently
// outlive the drawing it was run on. Development evidence only: no calibration, no M5 batch
// reporting, no M6 validity.
#include "t_junction_network.hpp"
#include "../src/project/evaluation.hpp"
#include "../src/project/run.hpp"
#include "../src/core/simulation.hpp"
#include <nlohmann/json.hpp>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace trafficsim::sweep {
inline const std::vector<std::uint32_t> kSeeds{0, 42, 43, 4294967295u};
// Paired: gapTime 3/5/7 s at headway 7 m, then headway 3/7/12 m at gapTime 5 s. (5, 7) is shared.
inline const std::vector<std::pair<double, double>> kRules{{3, 7}, {5, 7}, {7, 7}, {5, 3}, {5, 12}};
// M3.2.7c: the headway arm again, on the congested variant in which headway can decide.
inline const std::vector<std::pair<double, double>> kHeadwayRules{{5, 3}, {5, 7}, {5, 12}};
// FNV-1a 64 of a file's bytes with carriage returns dropped, so a Windows checkout of the same
// text hashes the same.
inline std::string fnv1a(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) throw std::runtime_error("SWEEP_MISSING " + file.string());
    std::uint64_t h = 1469598103934665603ull;
    for (char c; in.get(c);) {
        if (c == '\r') continue;
        h ^= static_cast<unsigned char>(c); h *= 1099511628211ull;
    }
    std::ostringstream out; out << std::hex << h;
    return out.str();
}
// What a sweep result depends on, apart from the build: the fixture's timing and volumes, and the
// hash of every catalog the compile reads and of the committed project file.
inline nlohmann::ordered_json fixtureMetadata(const std::filesystem::path& root) {
    const auto t = fixture::tJunction();
    const auto& def = *t.document.definition;
    nlohmann::ordered_json j;
    j["fixture"] = "data/projects/t-junction-priority.traffic.json";
    j["projectHash"] = fnv1a(root / "data/projects/t-junction-priority.traffic.json");
    j["duration"] = def.duration; j["timeStep"] = def.timeStep;
    for (const auto& input : def.inputs)
        j["inputs"].push_back({{"route", input.routeId}, {"vehicleTypeId", input.vehicleTypeId},
                               {"vehiclesPerHour", input.vehiclesPerHour}, {"startTime", input.startTime}, {"endTime", input.endTime}});
    for (const auto* dir : {"vehicle-types", "driver-behaviour", "priority-rules", "evaluation"}) {
        std::vector<std::filesystem::path> files;
        for (const auto& e : std::filesystem::directory_iterator(root / "data" / dir)) files.push_back(e.path());
        std::sort(files.begin(), files.end());
        for (const auto& f : files) j["catalogs"][std::string(dir) + "/" + f.filename().string()] = fnv1a(f);
    }
    j["seeds"] = kSeeds;
    for (const auto& [gap, headway] : kRules) j["rules"].push_back({{"gapTime", gap}, {"headway", headway}});
    return j;
}
// The congested variant is the committed drawing plus one fixed-time head (TJunctionOptions::
// congestedMajor). It is not a file of its own, so what it adds is recorded exactly -- the head's
// place and phases, read back from the builder -- rather than a hash of computed geometry, which
// another compiler may round differently in the last bit.
inline nlohmann::ordered_json congestedMetadata(const std::filesystem::path& root) {
    auto j = fixtureMetadata(root);
    fixture::TJunctionOptions o; o.congestedMajor = true;
    const auto t = fixture::tJunction(o);
    j["variant"] = "congestedMajor";
    for (const auto& h : t.document.network.signalHeads) {
        const auto& p = *std::find_if(t.document.definition->signalPrograms.begin(), t.document.definition->signalPrograms.end(),
                                      [&](const auto& x) { return x.id == h.programId; });
        nlohmann::ordered_json phases;
        for (const auto& ph : p.phases) phases.push_back({{"duration", ph.duration}, {"color", static_cast<int>(ph.color)}});
        j["variantHeads"].push_back({{"link", h.lane.linkId}, {"position", h.position}, {"phases", phases}});
    }
    j.erase("rules");
    for (const auto& [gap, headway] : kHeadwayRules) j["rules"].push_back({{"gapTime", gap}, {"headway", headway}});
    return j;
}
struct SweepRow {
    std::uint32_t seed{}; double gapTime{}, headway{};
    MovementReport report;
};
inline SweepRow runOne(const std::filesystem::path& data, std::uint32_t seed, double gapTime, double headway,
                       bool congested = false) {
    fixture::TJunctionOptions o; o.gapTime = gapTime; o.headway = headway; o.congestedMajor = congested;
    const auto t = fixture::tJunction(o);
    const auto snapshot = compileDocument(t.document, data);
    MovementAccumulator m(evaluationSpec(t.document, snapshot, data));
    auto s = createSimulation(snapshot.scenario, seed);
    m.observe(s);
    while (s.tick < totalTicks(snapshot.scenario)) { s = stepSimulation(s); m.observe(s); }
    return {seed, gapTime, headway, m.report(s)};
}
}
