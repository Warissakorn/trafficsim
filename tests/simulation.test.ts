import { describe, expect, it } from 'vitest';
import { createHash } from 'node:crypto';
import { createSimulation, runSimulation, signalColorAt, stepSimulation } from '../src/core';
import type { Scenario, SimState } from '../src/core';
import { occupiedSpans } from '../src/core/routes';
import { integrate } from '../src/core/following';
import { createDemo } from '../src/model/demo';
import { summarize } from '../src/eval/summary';
import { straight, vehicle, withVehicles } from './fixtures';

function finish(initial: SimState): SimState {
  let state = initial;
  while (state.tick < Math.round(state.scenario.duration / state.scenario.timeStep)) state = stepSimulation(state);
  return state;
}

describe('deterministic simulation contract', () => {
  it('preserves the M0 reference trajectory fingerprint', () => {
    const hash = createHash('sha256');
    for (const event of runSimulation(createDemo().scenario, 42)) hash.update(JSON.stringify(event) + '\n');
    expect(hash.digest('hex')).toBe('0884f1002da991c5d3dd1604c6a28482b985fc2d81917782b835e75e0d8e019a');
  });

  it('replays the exact event stream for the same seed, and changes arrivals for another seed', () => {
    const scenario = createDemo().scenario;
    const a = [...runSimulation(scenario, 42)];
    expect([...runSimulation(scenario, 42)]).toEqual(a);
    expect([...runSimulation(scenario, 43)].filter(e => e.kind === 'departed'))
      .not.toEqual(a.filter(e => e.kind === 'departed'));
    expect(a.some(e => e.kind === 'arrived')).toBe(true);
  });

  it('canonicalizes unordered scenario collections and remains pure under repeated stepping', () => {
    const scenario = createDemo().scenario;
    const shuffled: Scenario = { ...scenario, segments: [...scenario.segments].reverse(), inputs: [...scenario.inputs].reverse(), routes: [...scenario.routes].reverse(), signalPrograms: [...scenario.signalPrograms].reverse(), signalHeads: [...scenario.signalHeads].reverse() };
    expect([...runSimulation(shuffled, 7, { includeMovementEvents: false })])
      .toEqual([...runSimulation(scenario, 7, { includeMovementEvents: false })]);
    const initial = createSimulation(scenario, 7);
    const before = JSON.stringify(initial);
    expect(stepSimulation(initial)).toEqual(stepSimulation(initial));
    expect(JSON.stringify(initial)).toBe(before);
    expect(Object.isFrozen(initial.scenario.inputs)).toBe(true);
    expect(initial.scenario).not.toBe(scenario);
  });

  it('matches streaming and interactive stepping and stops at the exact final tick', () => {
    const scenario = straight({ duration: 60 });
    let state = createSimulation(scenario, 0);
    const events = [...state.events];
    while (state.time < scenario.duration) {
      state = stepSimulation(state);
      events.push(...state.events);
    }
    expect(events).toEqual([...runSimulation(scenario, 0)]);
    expect(state.time).toBe(60);
    expect(state.tick).toBe(600);
    expect(stepSimulation(state)).toBe(state);
    expect(() => stepSimulation(state, 0.2)).toThrow('timeStep');
  });

  it('retains arrivals in the last subinterval as pending demand', () => {
    const initial = createSimulation(straight({ duration: 1, inputs: [{ ...straight().inputs[0]!, endTime: 1 }] }), 1);
    const finalStep = { ...initial, tick: 9, time: 0.9, inputs: [{ id: 'input', nextArrival: 0.95, queue: [] }] };
    const final = stepSimulation(finalStep);
    expect(final.inputs[0]!.queue[0]?.scheduledTime).toBe(0.95);
    expect(final.vehicles).toHaveLength(0);
    expect(final.nextVehicleId).toBeGreaterThan(1);
  });

  it('continues an active stream against its own snapshot after caller edits', () => {
    const scenario = straight({ duration: 60 });
    const control = [...runSimulation(scenario, 2, { includeMovementEvents: false })];
    const stream = runSimulation(scenario, 2, { includeMovementEvents: false });
    const first = stream.next();
    (scenario as { duration: number }).duration = 1;
    expect([first.value, ...stream]).toEqual(control);
  });
});

describe('vehicle motion and boundaries', () => {
  it('accelerates one vehicle to its desired speed without exceeding it', () => {
    let state = withVehicles(straight({ segments: [{ id: 'road', length: 2000, next: [] }] }), [vehicle(1, 0)]);
    for (let i = 0; i < 300; i++) {
      const previous = state.vehicles[0]!;
      state = stepSimulation(state);
      const current = state.vehicles[0]!;
      expect(current.distance).toBeGreaterThanOrEqual(previous.distance);
      expect(current.speed).toBeGreaterThanOrEqual(previous.speed - 1e-10);
      expect(current.speed).toBeLessThanOrEqual(current.desiredSpeed);
    }
    expect(state.vehicles[0]!.speed).toBeCloseTo(15, 4);
  });

  it('integrates a within-step stop without reversing', () => {
    expect(integrate(1, -8, 0.5)).toEqual({ speed: 0, distance: 0.0625 });
  });

  it('preserves leftover distance through multiple short connectors in a step', () => {
    const scenario = straight({
      segments: [{ id: 'road', length: 10, next: ['connector'] }, { id: 'connector', length: 0.1, next: ['exit'] }, { id: 'exit', length: 100, next: [] }],
      routes: [{ id: 'route', segmentIds: ['road', 'connector', 'exit'] }],
    });
    const state = stepSimulation(withVehicles(scenario, [vehicle(1, 9.5, 15)]));
    expect(state.vehicles[0]!.distance).toBe(11);
    expect(state.events.filter(e => e.kind === 'segment-entered').map(e => e.segmentId)).toEqual(['connector', 'exit']);
    expect(state.events.find(e => e.kind === 'moved')).toMatchObject({ segmentId: 'exit', position: 0.9000000000000004 });
  });

  it('looks through a connector and retains the leader tail on its upstream link', () => {
    const scenario = straight({
      segments: [{ id: 'road', length: 100, next: ['connector'] }, { id: 'connector', length: 2, next: ['exit'] }, { id: 'exit', length: 100, next: [] }],
      routes: [{ id: 'route', segmentIds: ['road', 'connector', 'exit'] }],
    });
    let state = withVehicles(scenario, [vehicle(1, 102.5, 0), vehicle(2, 95, 15)]);
    expect(occupiedSpans(scenario, state.vehicles).some(span => span.vehicleId === 1 && span.segmentId === 'road')).toBe(true);
    for (let i = 0; i < 80; i++) {
      state = stepSimulation(state);
      if (state.vehicles.length === 2) expect(state.vehicles[0]!.distance - 4.5 - state.vehicles[1]!.distance).toBeGreaterThanOrEqual(2 - 1e-8);
    }
  });
});

describe('signals and queued demand', () => {
  const signalScenario = () => straight({
    signalPrograms: [{ id: 'program', offset: 0, phases: [{ duration: 30, color: 'red' }, { duration: 90, color: 'green' }] }],
    signalHeads: [{ id: 'head', segmentId: 'road', position: 60, programId: 'program' }],
  });

  it('uses half-open signal phases, offsets and cyclic timing', () => {
    const program = { id: 'p', offset: 2, phases: [{ duration: 3, color: 'red' as const }, { duration: 5, color: 'green' as const }] };
    expect([0, 1, 6, 7, 14].map(time => signalColorAt(program, time))).toEqual(['red', 'green', 'red', 'red', 'red']);
  });

  it('stops before a mid-link red signal and discharges at green', () => {
    let state = withVehicles(signalScenario(), [vehicle(1, 0), vehicle(2, -10)]);
    while (state.time < 30) {
      state = stepSimulation(state);
      for (const item of state.vehicles) expect(item.distance).toBeLessThanOrEqual(60 + 1e-8);
    }
    expect(state.vehicles[0]!.speed).toBeLessThan(0.1);
    expect(finish(state).completed).toBe(2);
  });

  it('does not brake a vehicle that has already passed a head when it turns red', () => {
    const state = stepSimulation(withVehicles(signalScenario(), [vehicle(1, 61, 10)]));
    expect(state.vehicles[0]!.speed).toBeGreaterThan(10);
  });

  it('preserves the leader gap when a nearby red head imposes a different stop position', () => {
    const scenario = { ...signalScenario(), timeStep: 0.5, signalHeads: [{ id: 'head', segmentId: 'road', position: 5, programId: 'program' }] };
    const state = stepSimulation(withVehicles(scenario, [vehicle(1, 10.5), vehicle(2, 0, 15)]));
    expect(state.vehicles[1]!.distance).toBeLessThanOrEqual(4);
  });

  it('never skips red phases; a head at a connector exit is visible upstream', () => {
    const scenario = straight({
      segments: [{ id: 'road', length: 10, next: ['connector'] }, { id: 'connector', length: 0.1, next: ['exit'] }, { id: 'exit', length: 100, next: [] }],
      routes: [{ id: 'route', segmentIds: ['road', 'connector', 'exit'] }],
      signalPrograms: [{ id: 'p', offset: 0, phases: [{ duration: 120, color: 'red' }] }],
      signalHeads: [{ id: 'h', segmentId: 'exit', position: 0, programId: 'p' }],
    });
    const state = stepSimulation(withVehicles(scenario, [vehicle(1, 9.8, 15)]));
    expect(state.vehicles[0]!.distance).toBeLessThanOrEqual(10.1);
    expect(state.vehicles[0]!.speed).toBe(0);
    expect(state.events.some(e => e.kind === 'safety-clamp')).toBe(true);
  });

  it('preserves blocked arrivals, never overlaps vehicles, and reports unfinished demand', () => {
    const scenario = straight({
      duration: 60, segments: [{ id: 'road', length: 30, next: [] }],
      inputs: [{ id: 'input', routeId: 'route', vehicleTypeId: 'car', vehiclesPerHour: 1800, startTime: 0, endTime: 60 }],
      signalPrograms: [{ id: 'p', offset: 0, phases: [{ duration: 60, color: 'red' }] }],
      signalHeads: [{ id: 'h', segmentId: 'road', position: 20, programId: 'p' }],
    });
    let state = createSimulation(scenario, 3);
    while (state.time < 60) {
      state = stepSimulation(state);
      const ordered = [...state.vehicles].sort((a, b) => b.distance - a.distance);
      for (let i = 1; i < ordered.length; i++) expect(ordered[i - 1]!.distance - 4.5 - ordered[i]!.distance).toBeGreaterThanOrEqual(2 - 1e-8);
      for (const item of state.vehicles) expect(item.distance).toBeLessThanOrEqual(20);
    }
    const pending = state.inputs.reduce((sum, input) => sum + input.queue.length, 0);
    expect(pending).toBeGreaterThan(0);
    expect(state.completed + state.vehicles.length + pending).toBe(state.nextVehicleId - 1);
    expect(state.completed).toBe(0);
    expect(summarize([]).meanDelay).toBeNull();
  });

  it('handles zero demand without creating vehicles or producing NaN measurements', () => {
    const scenario = straight({ inputs: [{ ...straight().inputs[0]!, vehiclesPerHour: 0 }] });
    const state = finish(createSimulation(scenario, 42));
    expect(state.vehicles).toEqual([]);
    expect(state.completed).toBe(0);
    expect(state.nextVehicleId).toBe(1);
    expect(state.inputs[0]!.nextArrival).toBeNull();
  });
});
