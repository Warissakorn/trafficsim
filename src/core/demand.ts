import { random } from './random';
import type { InputState, PendingVehicle, Scenario, SimState } from './types';

export function initializeInputs(scenario: Scenario, initialRandomState: number): {
  inputs: InputState[]; randomState: number;
} {
  let randomState = initialRandomState;
  const inputs = scenario.inputs.map(input => {
    if (input.vehiclesPerHour === 0) return { id: input.id, nextArrival: null, queue: [] };
    const draw = random(randomState);
    randomState = draw.state;
    const arrival = input.startTime - Math.log(draw.value) * 3600 / input.vehiclesPerHour;
    return { id: input.id, nextArrival: arrival < input.endTime ? arrival : null, queue: [] };
  });
  return { inputs, randomState };
}

/** Arrivals are sampled when due, regardless of whether entry is blocked. */
export function generateArrivals(state: SimState): {
  inputs: InputState[]; randomState: number; nextVehicleId: number;
} {
  let randomState = state.randomState;
  let nextVehicleId = state.nextVehicleId;
  const draw = () => {
    const result = random(randomState);
    randomState = result.state;
    return result.value;
  };
  const inputs = state.inputs.map(current => {
    const input = state.scenario.inputs.find(item => item.id === current.id)!;
    const type = state.scenario.vehicleTypes.find(item => item.id === input.vehicleTypeId)!;
    const queue: PendingVehicle[] = [...current.queue];
    let nextArrival = current.nextArrival;
    while (nextArrival !== null && nextArrival <= state.time) {
      const desiredSpeed = type.desiredSpeed.min + draw() * (type.desiredSpeed.max - type.desiredSpeed.min);
      const gaussian = Math.sqrt(-2 * Math.log(draw())) * Math.cos(2 * Math.PI * draw());
      queue.push({
        id: nextVehicleId++, inputId: input.id, routeId: input.routeId, vehicleTypeId: input.vehicleTypeId,
        scheduledTime: nextArrival, desiredSpeed, driverFactor: Math.max(0, Math.min(1, 0.5 + 0.15 * gaussian)),
      });
      const arrival: number = nextArrival - Math.log(draw()) * 3600 / input.vehiclesPerHour;
      nextArrival = arrival < input.endTime ? arrival : null;
    }
    return { id: current.id, nextArrival, queue };
  });
  return { inputs, randomState, nextVehicleId };
}
