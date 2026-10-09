#pragma once
#include "../core/types.hpp"
#include <optional>
#include <string>
#include <vector>

namespace trafficsim {
// A place on one runtime segment, metres along it: a queue counter's line, or one lane of a
// travel-time section's line.
struct CounterLine { std::string segmentId; double position{}; bool operator==(const CounterLine&) const = default; };
// M5.4 (D133, docs/reference/TRAVEL_TIME_SECTIONS.md): one section, its lines as the places on
// every lane they cross. NOT HCM control delay, NOT LOS, not validated (rule 4).
struct SectionSpec {
    std::string name; std::vector<CounterLine> start, end;
    // M5.5 (D134): carried to the row for LOS -- the pack's control type and the approach (start
    // Link) the section is grouped under. Neither changes what is measured.
    std::optional<std::string> controlType; std::string approach;
};
struct SectionRow {
    std::string name; std::uint64_t vehicles{};
    std::optional<double> meanTravelTime, meanDelay;
    std::uint64_t unfinished{};
    std::optional<std::string> controlType; std::string approach; // M5.5, from the spec
    bool operator==(const SectionRow&) const = default;
};
// Section crossings from SimState snapshots alone; MovementAccumulator owns one and calls it once
// per observed state, after it has applied that state's events.
class SectionAccumulator {
public:
    // A trip counts when its end crossing lies in [warmup, end] (no end: the end of the run).
    SectionAccumulator(std::vector<SectionSpec> sections, double warmup, std::optional<double> end);
    void observe(const SimState& state);
    std::vector<SectionRow> report() const;
private:
    struct Track {
        std::uint64_t id{}; std::uint32_t route{}; double distance{}, speed{}, time{}, desiredSpeed{};
        std::vector<double> open; // per section: the start crossing time, NaN when no trip is open
    };
    void bind(const SimState& state);
    void close(std::size_t k, Track& track, std::uint32_t route, double time);
    std::vector<SectionSpec> sections_;
    double warmup_{}; std::optional<double> periodEnd_;
    const Scenario* bound_{};
    // Per section, per route slot: the lines' route distances, NaN where the section does not apply.
    std::vector<std::vector<double>> start_, finish_;
    std::vector<double> routeLength_;
    std::vector<Track> tracks_; // sorted by vehicle id
    std::vector<std::uint64_t> count_;
    std::vector<double> travel_, delay_;
};
}
