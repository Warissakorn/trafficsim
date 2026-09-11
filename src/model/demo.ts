import fixture from '../../data/scenarios/crossing.json';
import behaviour from '../../data/driver-behaviour/default.json';
import car from '../../data/vehicle-types/car.json';
import { compileScenario } from './network';
import type { Network, ScenarioDefinition } from './network';

/** Built-in data is validated by the same compiler used by future project callers. */
export function createDemo() {
  const network = structuredClone(fixture.network) as Network;
  const definition: ScenarioDefinition = {
    ...fixture.definition,
    signalPrograms: fixture.definition.signalPrograms as ScenarioDefinition['signalPrograms'],
    vehicleTypes: [car], behaviours: [behaviour],
  };
  return { network, scenario: compileScenario(network, definition) };
}
