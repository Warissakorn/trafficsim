import { describe, expect, it } from 'vitest';
import { compileScenario, laneGeometry, pointAlong, polylineLength, validateNetwork } from '../src/model/network';
import type { Network } from '../src/model/network';
import { createDemo } from '../src/model/demo';
import { straight } from './fixtures';

describe('link-based network model', () => {
  it('compiles lanes and real connectors with SI lengths and mid-link signal heads', () => {
    const { network, scenario } = createDemo();
    expect(validateNetwork(network)).toEqual([]);
    expect(scenario.segments).toHaveLength(6);
    expect(scenario.segments.find(s => s.id === 'west-1')).toEqual({ id: 'west-1', length: 150, next: ['west-east'] });
    expect(scenario.segments.find(s => s.id === 'west-east')).toEqual({ id: 'west-east', length: 20, next: ['east-1'] });
    expect(scenario.signalHeads.find(s => s.id === 'west-head')?.position).toBe(148);
  });

  it('interpolates polylines by distance and clamps endpoints', () => {
    const points = [{ x: 0, y: 0 }, { x: 3, y: 4 }, { x: 3, y: 14 }];
    expect(polylineLength(points)).toBe(15);
    expect(pointAlong(points, 2.5)).toEqual({ x: 1.5, y: 2 });
    expect(pointAlong(points, 10)).toEqual({ x: 3, y: 9 });
    expect(pointAlong(points, -1)).toEqual(points[0]);
    expect(pointAlong(points, 100)).toEqual(points[2]);
    expect(() => pointAlong(points, NaN)).toThrow();
  });

  it('makes driving side determine lane geometry and compiled lane length', () => {
    const link = { id: 'link', geometry: [{ x: 0, y: 0 }, { x: 10, y: 0 }], lanes: [{ id: 'curb', width: 4 }, { id: 'inner', width: 4 }] };
    expect(laneGeometry(link, 'curb', 'left')).toEqual([{ x: 0, y: 2 }, { x: 10, y: 2 }]);
    expect(laneGeometry(link, 'curb', 'right')).toEqual([{ x: 0, y: -2 }, { x: 10, y: -2 }]);
    const network: Network = { id: 'two-lane', drivingSide: 'right', links: [link], connectors: [], signalHeads: [] };
    const definition = straight({ inputs: [], routes: [{ id: 'r', segmentIds: ['curb'] }] });
    expect(compileScenario(network, definition).segments.map(s => s.length)).toEqual([10, 10]);
  });

  it('rejects duplicate IDs, invalid geometry, dangling references and disconnected endpoints', () => {
    const { network } = createDemo();
    const broken: Network = {
      ...network,
      links: [...network.links, network.links[0]!],
      connectors: [{ ...network.connectors[0]!, to: { linkId: 'missing', laneId: 'bad' }, geometry: [{ x: 0, y: 0 }, { x: 0, y: 0 }] }],
    };
    const codes = validateNetwork(broken).map(issue => issue.code);
    expect(codes).toEqual(expect.arrayContaining(['DUPLICATE_ID', 'INVALID_GEOMETRY', 'UNKNOWN_LANE', 'DISCONNECTED_GEOMETRY']));
    expect(() => compileScenario(broken, straight())).toThrow('UNKNOWN_LANE');
  });

  it('rejects NaN widths and signal positions outside lane geometry', () => {
    const { network } = createDemo();
    const changed: Network = {
      ...network,
      links: network.links.map((link, i) => i ? link : { ...link, lanes: [{ ...link.lanes[0]!, width: NaN }] }),
      signalHeads: [{ ...network.signalHeads[0]!, position: -1 }],
    };
    expect(validateNetwork(changed).map(i => i.code)).toEqual(expect.arrayContaining(['INVALID_WIDTH', 'INVALID_POSITION']));
  });

  it('snapshots definition data instead of sharing it with a future editor', () => {
    const { network, scenario } = createDemo();
    const editable = structuredClone(scenario) as { inputs: { vehiclesPerHour: number }[] } & typeof scenario;
    const compiled = compileScenario(network, editable);
    editable.inputs[0]!.vehiclesPerHour = 1;
    expect(compiled.inputs[0]!.vehiclesPerHour).not.toBe(1);
  });
});
