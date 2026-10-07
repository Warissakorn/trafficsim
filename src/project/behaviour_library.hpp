#pragma once
#include "document.hpp"

namespace trafficsim {
// M3.3.2a (D126, docs/reference/DRIVING_BEHAVIOUR.md §1): the project-owned behaviour library --
// behaviour names and model tags, vehicle classes, link behaviour types -- and the Link/Connector
// assignment. Schema 21. Stored and validated only; nothing reaches the engine before M3.3.2b.
bool usesBehaviourLibrary(const ProjectDocument&); // anything that needs schema 21 to be saved
// A file below schema 21 carrying a library key is refused (EDIT_UNSUPPORTED_FIELD), never dropped.
void rejectBehaviourLibraryBefore21(const Json& document);
// Schema 21 and later: reads the library and rejects unknown keys in every owned catalog entry.
void parseBehaviourLibrary(const Json& definition, AuthoringDefinition&);
// Adds the schema-21 keys to an already written definition: names, the explicit model tag on
// every owned behaviour, classes and behaviour types (the last two only when non-empty).
void addBehaviourLibraryJson(const AuthoringDefinition&, Json& definition);
// Every rule applies to unused entries too. Empty when the document uses no library feature.
std::vector<ValidationIssue> behaviourLibraryIssues(const ProjectDocument&);
// Until M3.3.2b selects behaviour by road, an assigned road would be a silent no-op: Run refuses it.
std::vector<ValidationIssue> behaviourAssignmentIssues(const Network&);
inline constexpr const char* kPrototypeBehaviourModel = "prototype";
}
