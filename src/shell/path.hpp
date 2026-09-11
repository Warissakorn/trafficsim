#pragma once
#include <QString>
#include <filesystem>

namespace trafficsim {
inline std::filesystem::path nativePath(const QString& text) {
#ifdef _WIN32
    return std::filesystem::path(text.toStdWString());
#else
    return std::filesystem::path(text.toStdString());
#endif
}
inline QString displayPath(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    return QString::fromStdString(path.string());
#endif
}
}
