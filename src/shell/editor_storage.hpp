#pragma once
#include "../project/document.hpp"
#include <QString>
namespace trafficsim {
ProjectDocument readEditorDocument(const QString&);
void writeEditorDocument(const QString&,const Json&);
// Atomic replacement of any file the shell writes; throws errorCode (a locale key) on failure.
void writeEditorBytes(const QString& file,const std::string& bytes,const char* errorCode);
}
