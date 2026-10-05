#pragma once
#include "run.hpp"
namespace trafficsim {
struct DemandPreviewRow {
    std::string inputId,vehicleTypeId,routeId,entryLinkId,entryLaneId,lastLinkId;
    double startTime{},endTime{},vehiclesPerHour{},expectedVehicles{};
};
struct DemandPreview {
    std::uint64_t revision{};
    std::vector<DemandPreviewRow> rows;
    std::vector<ValidationIssue> advisories;
    double expectedVehicles{};
};
// Compile the same snapshot as Run, without generating arrivals or advancing simulation.
// Expected counts integrate the compiled rate only over the run horizon [0,duration).
DemandPreview previewDemand(const ProjectDocument&,const std::filesystem::path& dataDirectory);
}
