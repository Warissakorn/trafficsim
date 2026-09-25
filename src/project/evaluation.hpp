#pragma once
#include "run.hpp"
#include "../eval/movement.hpp"
#include "json.hpp"
#include <filesystem>

namespace trafficsim {
// data/evaluation/queue-counter.json in SI units. Throws EDIT_CATALOG_READ when it is missing or
// malformed: a queue figure measured against an invented threshold is not one to report.
QueueDefinition loadQueueDefinition(const std::filesystem::path& dataDirectory);
// Movements and queue counters for a compiled document (M2.5).
//  - A movement is an authored route's (first Link, last Link) pair, named from the Links' Names,
//    in authored route order; two routes with the same pair are one movement. Every runtime route
//    maps through its authored id, the prefix before the first '/'.
//  - A queue counter is an approach: the signal heads on one Link, one per lane, in Link order.
EvaluationSpec evaluationSpec(const ProjectDocument&, const RunSnapshot&, const std::filesystem::path& dataDirectory);
// M3.2.6c: what the editor's Queue counters tab shows, from the rules evaluationSpec applies.
// An authored counter's row is its name, or its id when unnamed; it replaces the derived row of
// every Link whose heads it measures (by id, in Link order).
std::string queueRowName(const AuthoredQueueCounter&);
std::vector<std::string> replacedApproaches(const Network&, const AuthoredQueueCounter&);
// The report as JSON (CLI), with the same honesty labels.
Json movementJson(const MovementReport&);
// The report as CSV. The first line states what the numbers are not.
std::string movementCsv(const MovementReport&);
}
