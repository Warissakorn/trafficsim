#pragma once
#include <QStringList>
#include <functional>
#include <optional>
#include <vector>
class QWidget;
namespace trafficsim {
// Local dialog values only; the caller commits the returned rows with its usual history command.
std::optional<std::vector<std::vector<double>>> editDemandPeriods(QWidget*,const QStringList&,
    const std::vector<std::vector<double>>&,const std::function<QString(const char*)>&,
    const std::function<const char*(const std::vector<std::vector<double>>&)>& validate = {});
}
