import { laneGeometry, polylineLength } from './geometry';
import type { LaneReference, Network, NetworkIssue, Point } from './types';

export function validateNetwork(network: Network): NetworkIssue[] {
  const issues: NetworkIssue[] = [];
  const add = (code: string, path: string) => issues.push({ code, path });
  const ids = new Set<string>();
  const id = (value: string, path: string) => {
    if (typeof value !== 'string' || value.trim() === '') add('INVALID_ID', path);
    else if (ids.has(value)) add('DUPLICATE_ID', path);
    else ids.add(value);
  };
  const geometry = (points: readonly Point[], path: string) => {
    if (points.length < 2 || points.some(p => !Number.isFinite(p.x) || !Number.isFinite(p.y)) ||
        !Number.isFinite(polylineLength(points)) || polylineLength(points) <= 0 ||
        points.some((p, i) => i > 0 && p.x === points[i - 1]!.x && p.y === points[i - 1]!.y)) {
      add('INVALID_GEOMETRY', path);
    }
  };
  const resolve = (ref: LaneReference, path: string) => {
    const link = network.links.find(item => item.id === ref.linkId);
    if (!link || !link.lanes.some(lane => lane.id === ref.laneId)) {
      add('UNKNOWN_LANE', path);
      return undefined;
    }
    return link;
  };
  id(network.id, 'id');
  if (!['left', 'right'].includes(network.drivingSide)) add('INVALID_DRIVING_SIDE', 'drivingSide');
  if (network.links.length === 0) add('EMPTY_NETWORK', 'links');
  network.links.forEach((link, i) => {
    const path = `links[${i}]`;
    id(link.id, `${path}.id`);
    geometry(link.geometry, `${path}.geometry`);
    if (link.lanes.length === 0) add('NO_LANES', `${path}.lanes`);
    link.lanes.forEach((lane, j) => {
      id(lane.id, `${path}.lanes[${j}].id`);
      if (!Number.isFinite(lane.width) || lane.width <= 0) add('INVALID_WIDTH', `${path}.lanes[${j}].width`);
    });
  });
  const connections = new Set<string>();
  network.connectors.forEach((connector, i) => {
    const path = `connectors[${i}]`;
    id(connector.id, `${path}.id`);
    geometry(connector.geometry, `${path}.geometry`);
    const from = resolve(connector.from, `${path}.from`);
    const to = resolve(connector.to, `${path}.to`);
    const pair = JSON.stringify([connector.from.laneId, connector.to.laneId]);
    if (connections.has(pair)) add('DUPLICATE_CONNECTION', path);
    connections.add(pair);
    for (const [link, ref, endpoint, end] of [
      [from, connector.from, connector.geometry[0], true],
      [to, connector.to, connector.geometry.at(-1), false],
    ] as const) {
      if (!link || !endpoint || link.geometry.length < 2) continue;
      const points = laneGeometry(link, ref.laneId, network.drivingSide);
      const expected = end ? points.at(-1)! : points[0]!;
      if (Math.hypot(endpoint.x - expected.x, endpoint.y - expected.y) > 0.01) {
        add('DISCONNECTED_GEOMETRY', `${path}.${end ? 'from' : 'to'}`);
      }
    }
  });
  network.signalHeads.forEach((head, i) => {
    const path = `signalHeads[${i}]`;
    id(head.id, `${path}.id`);
    const link = resolve(head.lane, `${path}.lane`);
    if (typeof head.programId !== 'string' || !head.programId.trim()) add('INVALID_ID', `${path}.programId`);
    if (!Number.isFinite(head.position) || head.position < 0 || (link &&
        head.position > polylineLength(laneGeometry(link, head.lane.laneId, network.drivingSide)))) {
      add('INVALID_POSITION', `${path}.position`);
    }
  });
  return issues;
}

export class NetworkValidationError extends Error {
  constructor(readonly issues: readonly NetworkIssue[]) {
    super(issues.map(issue => `${issue.code}: ${issue.path}`).join('\n'));
    this.name = 'NetworkValidationError';
  }
}

export function assertValidNetwork(network: Network): void {
  const issues = validateNetwork(network);
  if (issues.length) throw new NetworkValidationError(issues);
}
