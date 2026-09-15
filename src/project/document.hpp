#pragma once
#include "json.hpp"
#include "../model/demand/definition.hpp"

namespace trafficsim {
struct BackgroundImage {
    std::shared_ptr<const std::string> pngBase64{std::make_shared<const std::string>()};
    double x{}, y{}, metresPerPixel{1}, rotation{}, opacity{0.5};
    // Compares the image bytes, not the pointer: two documents holding equal images are equal
    // however the shared buffer was produced.
    bool operator==(const BackgroundImage& other) const {
        const bool same = pngBase64 == other.pngBase64 ||
            (pngBase64 && other.pngBase64 && *pngBase64 == *other.pngBase64);
        return same && x == other.x && y == other.y && metresPerPixel == other.metresPerPixel &&
            rotation == other.rotation && opacity == other.opacity;
    }
};
struct ProjectDocument {
    Network network{"network", DrivingSide::left, {}, {}, {}};
    std::optional<AuthoringDefinition> definition; // Typed authoring values, never a compiled scenario.
    BackgroundImage background;
    std::uint64_t nextId{1};
    std::uint64_t revision{};
    bool operator==(const ProjectDocument&) const = default;
};
AuthoringDefinition parseAuthoringDefinition(const Json&);
Json definitionJson(const AuthoringDefinition&);
void validateAuthoredDemand(const ProjectDocument&);
Json documentJson(const ProjectDocument& document);
ProjectDocument parseDocument(const Json& json);
void validateDocument(const ProjectDocument& document); // Empty networks are valid drafts.
std::string allocateId(ProjectDocument& document, const std::string& prefix);
}
