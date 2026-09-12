#pragma once
#include "json.hpp"

namespace trafficsim {
struct BackgroundImage {
    std::shared_ptr<const std::string> pngBase64{std::make_shared<const std::string>()};
    double x{}, y{}, metresPerPixel{1}, rotation{}, opacity{0.5};
};
struct ProjectDocument {
    Network network{"network", DrivingSide::left, {}, {}, {}};
    Json definition = nullptr; // Optional M0 authoring definition, never a compiled scenario.
    BackgroundImage background;
    std::uint64_t nextId{1};
    std::uint64_t revision{};
};
Json documentJson(const ProjectDocument& document);
ProjectDocument parseDocument(const Json& json);
void validateDocument(const ProjectDocument& document); // Empty networks are valid drafts.
std::string allocateId(ProjectDocument& document, const std::string& prefix);
}
