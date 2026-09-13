#pragma once
#include "types.hpp"
#include <stdexcept>

namespace trafficsim {
class ValidationError : public std::runtime_error {
public:
    explicit ValidationError(std::vector<ValidationIssue> issues);
    const std::vector<ValidationIssue> issues;
};
bool onTimeGrid(double value, double timeStep);
std::vector<ValidationIssue> validateScenario(const Scenario& scenario);
void assertValidScenario(const Scenario& scenario);
}
