#pragma once
#include "../project/demand_catalog.hpp"
#include <QString>
#include <functional>
class QWidget;
namespace trafficsim {
using CatalogText=std::function<QString(const char*)>;
bool editCatalogType(QWidget*,VehicleType&,std::string& name,const std::vector<DriverBehaviour>&,const CatalogText&);
bool editCatalogComposition(QWidget*,Composition&,const std::vector<VehicleType>&,const CatalogText&);
}
