import type { Scenario, SimState, Vehicle } from '../src/core';
import { createSimulation } from '../src/core';
import behaviour from '../data/driver-behaviour/default.json';
import car from '../data/vehicle-types/car.json';

export function straight(overrides: Partial<Scenario> = {}): Scenario {
  return {
    duration: 120, timeStep: 0.1,
    segments: [{ id: 'road', length: 150, next: [] }],
    routes: [{ id: 'route', segmentIds: ['road'] }],
    vehicleTypes: [car], behaviours: [behaviour],
    inputs: [{ id: 'input', routeId: 'route', vehicleTypeId: 'car', vehiclesPerHour: 900, startTime: 0, endTime: 60 }],
    signalPrograms: [], signalHeads: [], ...overrides,
  };
}

export function vehicle(id: number, distance: number, speed = 0, routeId = 'route'): Vehicle {
  return {
    id, distance, speed, routeId, inputId: 'input', vehicleTypeId: 'car', enteredTime: 0,
    scheduledTime: 0, desiredSpeed: 15, driverFactor: 0.5, acceleration: 0, mode: 'free',
  };
}

export function withVehicles(scenario: Scenario, vehicles: readonly Vehicle[]): SimState {
  const initial = createSimulation({ ...scenario, inputs: [] }, 42);
  return { ...initial, vehicles, nextVehicleId: Math.max(0, ...vehicles.map(item => item.id)) + 1 };
}
