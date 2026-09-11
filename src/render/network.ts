import { locateVehicle, signalColorAt } from '../core';
import type { SimState } from '../core';
import { laneGeometry, pointAlong } from '../model/network';
import type { Network, Point } from '../model/network';

/** Passive canvas adapter. Simulation time and state are owned by the caller. */
export function drawNetwork(canvas: HTMLCanvasElement, network: Network, state: SimState): void {
  const context = canvas.getContext('2d');
  if (!context) return;
  const paths = new Map<string, readonly Point[]>();
  for (const link of network.links) {
    for (const lane of link.lanes) paths.set(lane.id, laneGeometry(link, lane.id, network.drivingSide));
  }
  for (const connector of network.connectors) paths.set(connector.id, connector.geometry);
  const points = [...paths.values()].flat();
  const minX = Math.min(...points.map(p => p.x)) - 20;
  const maxX = Math.max(...points.map(p => p.x)) + 20;
  const minY = Math.min(...points.map(p => p.y)) - 20;
  const maxY = Math.max(...points.map(p => p.y)) + 20;
  const ratio = window.devicePixelRatio || 1;
  const box = canvas.getBoundingClientRect();
  canvas.width = Math.round(box.width * ratio);
  canvas.height = Math.round(box.height * ratio);
  const scale = Math.min(canvas.width / (maxX - minX), canvas.height / (maxY - minY));
  const project = (p: Point) => ({
    x: canvas.width / 2 + (p.x - (minX + maxX) / 2) * scale,
    y: canvas.height / 2 - (p.y - (minY + maxY) / 2) * scale,
  });
  context.fillStyle = '#101a2a';
  context.fillRect(0, 0, canvas.width, canvas.height);
  for (const [id, geometry] of paths) {
    context.beginPath();
    geometry.forEach((point, i) => {
      const p = project(point);
      if (i === 0) context.moveTo(p.x, p.y); else context.lineTo(p.x, p.y);
    });
    context.strokeStyle = network.connectors.some(c => c.id === id) ? '#314059' : '#475670';
    context.lineCap = 'butt';
    context.lineWidth = Math.max(4 * scale, 3 * ratio);
    context.stroke();
  }
  for (const head of state.scenario.signalHeads) {
    const position = project(pointAlong(paths.get(head.segmentId)!, head.position));
    const program = state.scenario.signalPrograms.find(p => p.id === head.programId)!;
    const color = signalColorAt(program, state.time);
    context.fillStyle = { red: '#ff6767', amber: '#f6c85f', green: '#54df9e' }[color];
    context.beginPath();
    context.arc(position.x, position.y, 5 * ratio, 0, 2 * Math.PI);
    context.fill();
  }
  for (const vehicle of state.vehicles) {
    const location = locateVehicle(state.scenario, vehicle);
    const position = project(pointAlong(paths.get(location.segmentId)!, location.position));
    context.fillStyle = vehicle.speed < 0.2 ? '#f6c85f' : '#96d7ff';
    context.beginPath();
    context.arc(position.x, position.y, Math.max(2.2 * ratio, 1.6 * scale), 0, 2 * Math.PI);
    context.fill();
  }
}
