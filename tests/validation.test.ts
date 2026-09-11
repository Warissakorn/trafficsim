import { expect, it } from 'vitest';
import { createSimulation, validateScenario } from '../src/core';
import { straight } from './fixtures';

it('rejects invalid seeds, unsafe timesteps and off-grid duration', () => {
  for (const seed of [-1, 2.5, NaN, Infinity, 0x100000000]) expect(() => createSimulation(straight(), seed)).toThrow();
  for (const timeStep of [0, -1, 1, NaN, Infinity]) expect(() => createSimulation(straight({ timeStep }), 1)).toThrow();
  expect(() => createSimulation(straight({ duration: 1.05, inputs: [] }), 1)).toThrow('OFF_TIME_GRID');
});

it('rejects disconnected routes, cyclic routes and unsafe merges before stepping', () => {
  expect(validateScenario(straight({ routes: [{ id: 'route', segmentIds: ['road', 'missing'] }] })).map(i => i.code))
    .toEqual(expect.arrayContaining(['UNKNOWN_SEGMENT', 'DISCONNECTED_ROUTE']));
  expect(() => createSimulation(straight({ routes: [{ id: 'route', segmentIds: ['road', 'road'] }] }), 1)).toThrow('UNSUPPORTED_ROUTE_CYCLE');
  const scenario = straight({ segments: [{ id: 'road', length: 10, next: ['exit'] }, { id: 'other', length: 10, next: ['exit'] }, { id: 'exit', length: 10, next: [] }] });
  expect(() => createSimulation(scenario, 1)).toThrow('UNSUPPORTED_MERGE');
});

it('rejects signal programs that would change between simulation ticks', () => {
  const scenario = straight({ signalPrograms: [{ id: 'p', offset: 0.05, phases: [{ duration: 0.15, color: 'red' }] }] });
  expect(validateScenario(scenario).filter(i => i.code === 'OFF_TIME_GRID')).toHaveLength(2);
});

it('rejects invalid parameter ranges and unknown demand/control references', () => {
  const initial = straight();
  const scenario = straight({
    vehicleTypes: [{ ...initial.vehicleTypes[0]!, desiredSpeed: { min: 20, max: 10 }, length: NaN }],
    inputs: [{ ...initial.inputs[0]!, routeId: 'bad', vehiclesPerHour: -1, endTime: 121 }],
    signalHeads: [{ id: 'head', segmentId: 'road', position: 999, programId: 'bad' }],
  });
  expect(validateScenario(scenario).map(i => i.code)).toEqual(expect.arrayContaining(['INVALID_RANGE', 'INVALID_NUMBER', 'UNKNOWN_ROUTE', 'INVALID_INTERVAL', 'UNKNOWN_SIGNAL_PROGRAM', 'INVALID_POSITION']));
});
