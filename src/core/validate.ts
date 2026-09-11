import type { Scenario, ValidationIssue } from './types';

export function onTimeGrid(value: number, timeStep: number): boolean {
  const ticks = value / timeStep;
  return Number.isFinite(ticks) && Number.isSafeInteger(Math.round(ticks)) &&
    Math.abs(ticks - Math.round(ticks)) < 1e-7;
}

/** Validate runtime data as well as authoring output; no external model dependency. */
export function validateScenario(scenario: Scenario): ValidationIssue[] {
  const issues: ValidationIssue[] = [];
  const add = (code: string, path: string) => issues.push({ code, path });
  const number = (value: number, path: string, allowZero = false) => {
    if (!Number.isFinite(value) || (allowZero ? value < 0 : value <= 0)) add('INVALID_NUMBER', path);
  };
  const index = <T extends { readonly id: string }>(items: readonly T[], path: string) => {
    const map = new Map<string, T>();
    items.forEach((item, i) => {
      if (typeof item.id !== 'string' || !item.id.trim()) add('INVALID_ID', `${path}[${i}].id`);
      else if (map.has(item.id)) add('DUPLICATE_ID', `${path}[${i}].id`);
      map.set(item.id, item);
    });
    return map;
  };
  number(scenario.timeStep, 'timeStep');
  if (scenario.timeStep > 0.5) add('TIMESTEP_TOO_LARGE', 'timeStep');
  number(scenario.duration, 'duration');
  if (!onTimeGrid(scenario.duration, scenario.timeStep)) add('OFF_TIME_GRID', 'duration');
  const segments = index(scenario.segments, 'segments');
  const routes = index(scenario.routes, 'routes');
  const types = index(scenario.vehicleTypes, 'vehicleTypes');
  const behaviours = index(scenario.behaviours, 'behaviours');
  const programs = index(scenario.signalPrograms, 'signalPrograms');
  index(scenario.inputs, 'inputs');
  index(scenario.signalHeads, 'signalHeads');
  if (!segments.size) add('EMPTY_NETWORK', 'segments');
  const predecessors = new Map<string, number>();
  scenario.segments.forEach((segment, i) => {
    const path = `segments[${i}]`;
    number(segment.length, `${path}.length`);
    if (new Set(segment.next).size !== segment.next.length) add('DUPLICATE_CONNECTION', `${path}.next`);
    segment.next.forEach(next => {
      if (!segments.has(next)) add('UNKNOWN_SEGMENT', `${path}.next`);
      predecessors.set(next, (predecessors.get(next) ?? 0) + 1);
    });
  });
  // Without gap acceptance, accepting a merge would silently permit a collision.
  for (const [segment, count] of predecessors) {
    if (count > 1) add('UNSUPPORTED_MERGE', `segments.${segment}`);
  }
  scenario.routes.forEach((route, i) => {
    const path = `routes[${i}].segmentIds`;
    if (!route.segmentIds.length) add('EMPTY_ROUTE', path);
    if (new Set(route.segmentIds).size !== route.segmentIds.length) add('UNSUPPORTED_ROUTE_CYCLE', path);
    route.segmentIds.forEach((segmentId, j) => {
      const segment = segments.get(segmentId);
      if (!segment) add('UNKNOWN_SEGMENT', `${path}[${j}]`);
      const next = route.segmentIds[j + 1];
      if (segment && next !== undefined && !segment.next.includes(next)) add('DISCONNECTED_ROUTE', `${path}[${j + 1}]`);
    });
  });
  scenario.behaviours.forEach((behaviour, i) => {
    const path = `behaviours[${i}]`;
    number(behaviour.standstillDistance, `${path}.standstillDistance`);
    number(behaviour.additiveSafetyDistance, `${path}.additiveSafetyDistance`, true);
    number(behaviour.multiplicativeSafetyDistance, `${path}.multiplicativeSafetyDistance`, true);
    number(behaviour.followingTime, `${path}.followingTime`);
    number(behaviour.speedThreshold, `${path}.speedThreshold`);
  });
  scenario.vehicleTypes.forEach((type, i) => {
    const path = `vehicleTypes[${i}]`;
    for (const key of ['length', 'width', 'maxAcceleration', 'comfortableDeceleration', 'maxDeceleration'] as const) {
      number(type[key], `${path}.${key}`);
    }
    number(type.desiredSpeed.min, `${path}.desiredSpeed.min`);
    number(type.desiredSpeed.max, `${path}.desiredSpeed.max`);
    if (type.desiredSpeed.max < type.desiredSpeed.min) add('INVALID_RANGE', `${path}.desiredSpeed`);
    if (type.maxDeceleration < type.comfortableDeceleration) add('INVALID_RANGE', `${path}.maxDeceleration`);
    if (!behaviours.has(type.behaviourId)) add('UNKNOWN_BEHAVIOUR', `${path}.behaviourId`);
  });
  scenario.inputs.forEach((input, i) => {
    const path = `inputs[${i}]`;
    if (!routes.has(input.routeId)) add('UNKNOWN_ROUTE', `${path}.routeId`);
    if (!types.has(input.vehicleTypeId)) add('UNKNOWN_VEHICLE_TYPE', `${path}.vehicleTypeId`);
    number(input.vehiclesPerHour, `${path}.vehiclesPerHour`, true);
    number(input.startTime, `${path}.startTime`, true);
    number(input.endTime, `${path}.endTime`);
    if (input.startTime >= input.endTime || input.endTime > scenario.duration) add('INVALID_INTERVAL', path);
    // Source lanes cannot inject vehicles halfway into another route's occupied body.
    const first = routes.get(input.routeId)?.segmentIds[0];
    if (first && predecessors.has(first)) add('UNSUPPORTED_INTERNAL_INPUT', `${path}.routeId`);
  });
  scenario.signalPrograms.forEach((program, i) => {
    const path = `signalPrograms[${i}]`;
    if (!Number.isFinite(program.offset) || !onTimeGrid(program.offset, scenario.timeStep)) add('OFF_TIME_GRID', `${path}.offset`);
    if (!program.phases.length) add('EMPTY_SIGNAL_PROGRAM', `${path}.phases`);
    program.phases.forEach((phase, j) => {
      number(phase.duration, `${path}.phases[${j}].duration`);
      if (!onTimeGrid(phase.duration, scenario.timeStep)) add('OFF_TIME_GRID', `${path}.phases[${j}].duration`);
      if (!['red', 'amber', 'green'].includes(phase.color)) add('INVALID_SIGNAL_COLOR', `${path}.phases[${j}].color`);
    });
  });
  scenario.signalHeads.forEach((head, i) => {
    const path = `signalHeads[${i}]`;
    const segment = segments.get(head.segmentId);
    if (!segment) add('UNKNOWN_SEGMENT', `${path}.segmentId`);
    if (!programs.has(head.programId)) add('UNKNOWN_SIGNAL_PROGRAM', `${path}.programId`);
    number(head.position, `${path}.position`, true);
    if (segment && head.position > segment.length) add('INVALID_POSITION', `${path}.position`);
  });
  return issues;
}

export class ScenarioValidationError extends Error {
  constructor(readonly issues: readonly ValidationIssue[]) {
    super(issues.map(issue => `${issue.code}: ${issue.path}`).join('\n'));
    this.name = 'ScenarioValidationError';
  }
}

export function assertValidScenario(scenario: Scenario): void {
  const issues = validateScenario(scenario);
  if (issues.length) throw new ScenarioValidationError(issues);
}
