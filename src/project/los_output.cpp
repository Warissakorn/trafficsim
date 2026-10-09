#include "los_output.hpp"
#include "csv_format.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <ostream>
#include <stdexcept>

namespace trafficsim {
LosPack loadLosPack(const std::filesystem::path& dataDirectory, InputManifest* manifest) {
    try {
        const auto j = readInputJson(dataDirectory / "los" / "hcm.json", "los/hcm.json", manifest);
        for (const auto& [key, value] : j.items())
            if (key != "id" && key != "description" && key != "controlTypes") throw std::runtime_error("key");
        LosPack pack{j.at("id").get<std::string>(), {}};
        const auto& types = j.at("controlTypes");
        if (types.size() != 2) throw std::runtime_error("types");
        for (const char* type : {"signalised", "unsignalised"}) {
            const auto values = types.at(type).get<std::vector<double>>();
            if (values.size() != 5) throw std::runtime_error("count");
            std::array<double, 5> bounds{};
            for (std::size_t i = 0; i < 5; ++i) {
                if (!std::isfinite(values[i]) || values[i] <= 0 || (i && values[i] <= values[i - 1])) throw std::runtime_error("bounds");
                bounds[i] = values[i];
            }
            pack.bounds[type] = bounds;
        }
        return pack;
    } catch (const std::exception&) { throw std::runtime_error("EDIT_CATALOG_READ"); }
}
std::string losCell(const std::optional<double>& delay, const std::optional<std::string>& controlType,
                    const std::optional<LosPack>& pack) {
    if (!delay || !controlType || !pack) return {};
    const auto letter = losLetter(*delay, *controlType, *pack);
    return letter ? std::string(1, *letter) : std::string{};
}
namespace {
bool lettered(const std::vector<LosInput>& sections, const std::optional<LosPack>& pack) {
    return pack && std::any_of(sections.begin(), sections.end(), [](const auto& s) { return s.controlType.has_value(); });
}
}
void addLosJson(Json& j, const std::vector<LosInput>& sections, const std::optional<LosPack>& pack) {
    if (!lettered(sections, pack)) return;
    const auto groups = losGroups(sections, *pack);
    const auto rows = [](const std::vector<LosGroup>& gs) {
        Json out = Json::array();
        for (const auto& g : gs)
            out.push_back({{"name", g.name}, {"controlType", g.controlType}, {"vehicles", g.vehicles},
                           {"delay", g.delay ? Json(*g.delay) : Json(nullptr)},
                           {"los", g.los ? Json(std::string(1, *g.los)) : Json(nullptr)}});
        return out;
    };
    j["los"] = {{"pack", pack->id}, {"measure", "simulated section delay, not HCM control delay; not validated (M6)"},
                {"approaches", rows(groups.approaches)}, {"intersections", rows(groups.intersections)}};
}
void writeLosCsv(std::ostream& out, const std::vector<LosInput>& sections, const std::optional<LosPack>& pack) {
    if (!lettered(sections, pack)) return;
    const auto groups = losGroups(sections, *pack);
    out << "\n# LOS (pack " << pack->id << ") from simulated section delay, not HCM control delay; not validated (M6)\n";
    out << "level,name,controlType,vehicles,delay_s,los\n";
    const auto rows = [&](const char* level, const std::vector<LosGroup>& gs) {
        for (const auto& g : gs)
            out << level << ',' << csvQuoted(g.name) << ',' << g.controlType << ',' << csvNumber(g.vehicles) << ','
                << csvNumber(g.delay) << ',' << (g.los ? std::string(1, *g.los) : std::string{}) << '\n';
    };
    rows("approach", groups.approaches); rows("intersection", groups.intersections);
}
}
