import { createSimulation, stepSimulation } from '../src/core';
import type { SimEvent } from '../src/core';
import { summarize } from '../src/eval/summary';
import { createDemo } from '../src/model/demo';

const seed = Number(process.argv[2] ?? 42);
let state = createSimulation(createDemo().scenario, seed);
const events: SimEvent[] = [];
while (state.time < state.scenario.duration) {
  state = stepSimulation(state);
  events.push(...state.events.filter(event => event.kind !== 'moved'));
}
console.log(JSON.stringify({
  seed, simulatedSeconds: state.time, ...summarize(events),
  active: state.vehicles.length,
  pending: state.inputs.reduce((sum, input) => sum + input.queue.length, 0),
}, null, 2));
