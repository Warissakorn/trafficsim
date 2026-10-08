// M3.3.3a: the documents the W74 evidence tools run (BA27 sweep, clamp trace). The project's
// catalogs are captured as owned; the w74 arm adds the given behaviour and points every
// vehicle type at it. Shared so both tools run exactly the same inputs.
#pragma once
#include "../src/commands/behaviour_commands.hpp"
#include "../src/commands/catalog_commands.hpp"
#include "../src/commands/demand_commands.hpp"
#include "../src/project/demand_catalog.hpp"
#include "../src/project/document.hpp"
#include "../src/project/json.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
namespace trafficsim::w74fixture {
inline Json read(const std::filesystem::path& file) {
    std::ifstream in(file);
    if (!in) throw std::runtime_error("Cannot read " + file.string());
    return Json::parse(in);
}
inline DriverBehaviour behaviour(const std::filesystem::path& file) {
    auto w74 = parseBehaviour(read(file));
    if (!w74.w74) throw std::invalid_argument("The behaviour file must be a w74 behaviour");
    return w74;
}
inline ProjectDocument base(const std::filesystem::path& project, const std::filesystem::path& data) {
    auto d = parseDocument(read(project));
    putDemandCatalog(d, resolveDemandCatalog(*d.definition, data));
    return d;
}
// `w74` null: the prototype arm.
inline ProjectDocument arm(ProjectDocument d, const DriverBehaviour* w74, double dt) {
    const double duration = d.definition->duration;
    if (w74) {
        putBehaviour(d, *w74, "W74 discharge fixture");
        for (auto& t : d.definition->vehicleTypes) t.behaviourId = w74->id;
    }
    changeRunSettings(d, duration, dt);
    validateDocument(d);
    return d;
}
}
