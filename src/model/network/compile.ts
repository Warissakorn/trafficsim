import type { Scenario, Segment } from '../../core/types';
import { assertValidScenario } from '../../core/validate';
import { laneGeometry, polylineLength } from './geometry';
import type { Network } from './types';
import { assertValidNetwork } from './validate';

export type ScenarioDefinition = Omit<Scenario, 'segments' | 'signalHeads'>;

/** Derived runtime snapshot, never a second persisted network representation. */
export function compileScenario(network: Network, definition: ScenarioDefinition): Scenario {
  assertValidNetwork(network);
  const segments: Segment[] = network.links.flatMap(link => link.lanes.map(lane => ({
    id: lane.id,
    length: polylineLength(laneGeometry(link, lane.id, network.drivingSide)),
    next: network.connectors.filter(c => c.from.laneId === lane.id).map(c => c.id),
  })));
  segments.push(...network.connectors.map(connector => ({
    id: connector.id,
    length: polylineLength(connector.geometry),
    next: [connector.to.laneId],
  })));
  const scenario: Scenario = {
    ...definition,
    segments,
    signalHeads: network.signalHeads.map(head => ({
      id: head.id, segmentId: head.lane.laneId, position: head.position, programId: head.programId,
    })),
  };
  assertValidScenario(scenario);
  // The authoring model remains editable without changing a compiled scenario.
  return structuredClone(scenario);
}
