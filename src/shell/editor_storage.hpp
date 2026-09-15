#pragma once
#include "../project/document.hpp"
#include <QString>
namespace trafficsim {
ProjectDocument readEditorDocument(const QString&);
void writeEditorDocument(const QString&,const Json&);
}
