#pragma once
#include "movement.hpp"
#include "discharge_passage.hpp"
#include <set>

namespace trafficsim {
// M3.3.1: tick-quantized, per-head/lane queue discharge; not capacity, PCU or LOS.
struct DischargeSpec {
    double windowStart{}, windowEnd{}, warmup{};
    std::size_t steadyFirst{3}, steadyLast{5}, startupLast{2}; // one-based crossing ranks
    std::set<std::string> vehicleTypeIds; // empty: all; select follower gaps without reranking
};
struct DischargeCrossing {
    std::uint64_t vehicleId{};
    std::string vehicleTypeId;
    double time{};
    bool queuedAtGo{};
    bool operator==(const DischargeCrossing&) const = default;
};
struct DischargeCycle {
    std::string headId, laneId;
    double go{}, end{}, timeStep{};
    bool complete{};
    std::string unavailable;
    std::vector<DischargeCrossing> crossings;
};
struct DischargeEstimate {
    std::string reason;
    std::size_t samples{};
    std::optional<double> meanHeadway, dischargeVehiclesPerHour, startupLostTime;
    std::vector<std::size_t> sampledRanks;
    std::string startupUnavailableReason;
};
void validateDischargeSpec(const DischargeSpec&);
DischargeEstimate estimateDischarge(const DischargeCycle&, const DischargeSpec&);

class DischargeAccumulator {
public:
    DischargeAccumulator(DischargeSpec, QueueDefinition);
    // Every consecutive snapshot including createSimulation; repeated tick is a no-op.
    // Different Scenario, skipped/backward ticks and invalid specs reject.
    void observe(const SimState&);
    std::vector<DischargeCycle> report() const; // completed and partial cycles, in stable head order
private:
    struct Head {
        std::vector<double> atRoute;
        std::optional<DischargeCycle> cycle;
        std::set<std::uint64_t> queue;
    };
    DischargeSpec spec_;
    QueueDefinition queue_;
    std::shared_ptr<const Scenario> scenario_;
    std::optional<std::uint64_t> tick_;
    std::vector<Head> heads_;
    std::vector<DischargeCycle> closed_;
    DischargePositions previous_;
    std::map<std::uint64_t,std::size_t> pending_;
};
}
