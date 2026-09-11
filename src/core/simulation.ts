import { generateArrivals, initializeInputs } from './demand';
import { followingAcceleration, integrate } from './following';
import type { Leader } from './following';
import { normalizeSeed } from './random';
import { locateVehicle, occupiedSpans, routeParts } from './routes';
import type { OccupiedSpan, RoutePart } from './routes';
import { signalColorAt } from './signals';
import type { EventStream, RunOptions, Scenario, SimEvent, SimState, Vehicle } from './types';
import { assertValidScenario } from './validate';

function freeze<T>(value: T): T {
  if (value && typeof value === 'object' && !Object.isFrozen(value)) {
    for (const child of Object.values(value)) freeze(child);
    Object.freeze(value);
  }
  return value;
}

function canonicalScenario(source: Scenario): Scenario {
  const clone = JSON.parse(JSON.stringify(source)) as Scenario;
  const sorted = <T extends { readonly id: string }>(items: readonly T[]): T[] =>
    [...items].sort((a, b) => a.id < b.id ? -1 : a.id > b.id ? 1 : 0);
  return freeze({
    ...clone,
    segments: sorted(clone.segments).map(segment => ({ ...segment, next: [...segment.next].sort() })),
    routes: sorted(clone.routes), vehicleTypes: sorted(clone.vehicleTypes),
    behaviours: sorted(clone.behaviours), inputs: sorted(clone.inputs),
    signalPrograms: sorted(clone.signalPrograms), signalHeads: sorted(clone.signalHeads),
  });
}

export function createSimulation(scenario: Scenario, seed: number): SimState {
  assertValidScenario(scenario);
  const randomState = normalizeSeed(seed);
  const snapshot = canonicalScenario(scenario);
  const demand = initializeInputs(snapshot, randomState);
  return freeze({
    scenario: snapshot, seed, tick: 0, time: 0,
    ...demand, nextVehicleId: 1, vehicles: [], completed: 0,
    events: snapshot.signalHeads.map(head => ({
      kind: 'signal' as const, time: 0, signalId: head.id,
      color: signalColorAt(snapshot.signalPrograms.find(program => program.id === head.programId)!, 0),
    })),
  });
}

function closestVehicle(vehicle: Vehicle, parts: readonly RoutePart[], spans: readonly OccupiedSpan[]): Leader | undefined {
  let nearest: Leader | undefined;
  for (const part of parts) {
    if (part.start + part.length < vehicle.distance) continue;
    for (const span of spans) {
      if (span.vehicleId === vehicle.id || span.segmentId !== part.segmentId ||
          part.start + span.front < vehicle.distance - 1e-9) continue;
      const gap = part.start + span.rear - vehicle.distance;
      if (!nearest || gap < nearest.gap) nearest = { gap, speed: span.speed };
    }
  }
  return nearest;
}

/** Pure synchronous step. dt must match the fixed scenario timestep. */
export function stepSimulation(state: SimState, dt = state.scenario.timeStep): SimState {
  if (dt !== state.scenario.timeStep) throw new RangeError('dt must equal scenario.timeStep');
  if (state.tick >= Math.round(state.scenario.duration / dt)) return state;
  const { scenario } = state;
  const tick = state.tick + 1;
  const time = tick * dt;
  const events: SimEvent[] = [];
  const demand = generateArrivals(state);
  const vehicles: Vehicle[] = [...state.vehicles];
  const candidates = demand.inputs.flatMap(input => input.queue.length ? [input.queue[0]!] : [])
    .sort((a, b) => a.scheduledTime - b.scheduledTime || a.id - b.id);
  const attemptedSources = new Set<string>();
  for (const pending of candidates) {
    const route = scenario.routes.find(item => item.id === pending.routeId)!;
    const first = route.segmentIds[0]!;
    if (attemptedSources.has(first)) continue;
    attemptedSources.add(first);
    const type = scenario.vehicleTypes.find(item => item.id === pending.vehicleTypeId)!;
    const behaviour = scenario.behaviours.find(item => item.id === type.behaviourId)!;
    const vehicle: Vehicle = {
      ...pending, enteredTime: state.time, distance: 0, speed: 0, acceleration: 0, mode: 'free',
    };
    const leader = closestVehicle(vehicle, routeParts(scenario, route), occupiedSpans(scenario, vehicles));
    if (leader && leader.gap < behaviour.standstillDistance) continue;
    vehicles.push(vehicle);
    const inputIndex = demand.inputs.findIndex(input => input.id === pending.inputId);
    const input = demand.inputs[inputIndex]!;
    demand.inputs[inputIndex] = { ...input, queue: input.queue.slice(1) };
    events.push({
      kind: 'departed', time: state.time, vehicleId: vehicle.id, routeId: vehicle.routeId,
      scheduledTime: vehicle.scheduledTime, desiredSpeed: vehicle.desiredSpeed,
    });
  }
  vehicles.sort((a, b) => a.id - b.id);
  const spans = occupiedSpans(scenario, vehicles);
  const remaining: Vehicle[] = [];
  let completed = state.completed;
  for (const vehicle of vehicles) {
    const type = scenario.vehicleTypes.find(item => item.id === vehicle.vehicleTypeId)!;
    const behaviour = scenario.behaviours.find(item => item.id === type.behaviourId)!;
    const parts = routeParts(scenario, scenario.routes.find(route => route.id === vehicle.routeId)!);
    let leader = closestVehicle(vehicle, parts, spans);
    let allowedDistance = leader ? Math.max(0, leader.gap - behaviour.standstillDistance) : Infinity;
    for (const head of scenario.signalHeads) {
      const part = parts.find(item => item.segmentId === head.segmentId);
      if (!part) continue;
      const gap = part.start + head.position - vehicle.distance;
      if (gap < -1e-9) continue; // Already past the head: no retroactive braking.
      const program = scenario.signalPrograms.find(item => item.id === head.programId)!;
      if (signalColorAt(program, state.time) === 'green') continue;
      allowedDistance = Math.min(allowedDistance, Math.max(0, gap));
      if (!leader || gap < leader.gap) leader = { gap, speed: 0 };
    }
    const following = followingAcceleration(vehicle.speed, vehicle.desiredSpeed, vehicle.driverFactor, type, behaviour, leader);
    const motion = integrate(vehicle.speed, following.acceleration, dt);
    let speed = Math.min(vehicle.desiredSpeed, motion.speed);
    let distance = motion.distance;
    let acceleration = following.acceleration;
    if (distance > allowedDistance) {
      distance = allowedDistance;
      speed = 0;
      acceleration = -vehicle.speed / dt;
      events.push({ kind: 'safety-clamp', time, vehicleId: vehicle.id });
    }
    const moved: Vehicle = { ...vehicle, distance: vehicle.distance + distance, speed, acceleration, mode: following.mode };
    for (const part of parts.slice(1)) {
      if (vehicle.distance < part.start && moved.distance >= part.start) {
        events.push({ kind: 'segment-entered', time, vehicleId: vehicle.id, segmentId: part.segmentId });
      }
    }
    const last = parts.at(-1)!;
    const routeLength = last.start + last.length;
    if (moved.distance >= routeLength) {
      completed++;
      events.push({
        kind: 'arrived', time, vehicleId: vehicle.id, routeId: vehicle.routeId,
        travelTime: time - vehicle.enteredTime, departureDelay: vehicle.enteredTime - vehicle.scheduledTime,
        freeFlowTime: routeLength / vehicle.desiredSpeed,
      });
    } else {
      remaining.push(moved);
      events.push({ kind: 'moved', time, vehicleId: vehicle.id, ...locateVehicle(scenario, moved), speed, acceleration });
    }
  }
  for (const head of scenario.signalHeads) {
    const program = scenario.signalPrograms.find(item => item.id === head.programId)!;
    const color = signalColorAt(program, time);
    if (color !== signalColorAt(program, state.time)) events.push({ kind: 'signal', time, signalId: head.id, color });
  }
  const next = { ...state, ...demand, tick, time, vehicles: remaining, completed, events };
  // Arrivals in the final subinterval still belong to demand, even if no step
  // remains in which to insert them. Retain them in the external queue.
  if (tick === Math.round(scenario.duration / dt)) return freeze({ ...next, ...generateArrivals(next) });
  return freeze(next);
}

/** Streaming avoids retaining trajectory history in the state or coupling to I/O. */
export function* runSimulation(scenario: Scenario, seed: number, options: RunOptions = {}): EventStream {
  let state = createSimulation(scenario, seed);
  yield* state.events;
  while (state.tick < Math.round(state.scenario.duration / state.scenario.timeStep)) {
    state = stepSimulation(state);
    for (const event of state.events) {
      if (options.includeMovementEvents !== false || event.kind !== 'moved') yield event;
    }
  }
}
