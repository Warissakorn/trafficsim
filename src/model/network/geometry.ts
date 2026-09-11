import type { Link, Network, Point } from './types';

export function polylineLength(points: readonly Point[]): number {
  let length = 0;
  for (let i = 1; i < points.length; i++) {
    const a = points[i - 1]!;
    const b = points[i]!;
    length += Math.hypot(b.x - a.x, b.y - a.y);
  }
  return length;
}

/** Piecewise linear interpolation; endpoints clamp, invalid geometry is rejected. */
export function pointAlong(points: readonly Point[], distance: number): Point {
  if (points.length < 2 || !Number.isFinite(distance) ||
      points.some(p => !Number.isFinite(p.x) || !Number.isFinite(p.y)) ||
      polylineLength(points) <= 0) throw new RangeError('INVALID_GEOMETRY');
  let remaining = Math.max(0, distance);
  for (let i = 1; i < points.length; i++) {
    const a = points[i - 1]!;
    const b = points[i]!;
    const length = Math.hypot(b.x - a.x, b.y - a.y);
    if (length === 0) continue;
    if (remaining <= length) {
      const ratio = remaining / length;
      return { x: a.x + ratio * (b.x - a.x), y: a.y + ratio * (b.y - a.y) };
    }
    remaining -= length;
  }
  return { ...points[points.length - 1]! };
}

/** Bounded vertex-normal offsets. The link polyline is the road centreline. */
export function laneGeometry(link: Link, laneId: string, side: Network['drivingSide']): Point[] {
  const laneIndex = link.lanes.findIndex(lane => lane.id === laneId);
  if (laneIndex < 0) throw new RangeError('UNKNOWN_LANE');
  const totalWidth = link.lanes.reduce((sum, lane) => sum + lane.width, 0);
  const before = link.lanes.slice(0, laneIndex).reduce((sum, lane) => sum + lane.width, 0);
  const offset = (totalWidth / 2 - before - link.lanes[laneIndex]!.width / 2) *
    (side === 'left' ? 1 : -1);
  return link.geometry.map((point, i) => {
    const previous = link.geometry[Math.max(0, i - 1)]!;
    const next = link.geometry[Math.min(link.geometry.length - 1, i + 1)]!;
    let dx = next.x - previous.x;
    let dy = next.y - previous.y;
    if (dx === 0 && dy === 0) { dx = next.x - point.x; dy = next.y - point.y; }
    const norm = Math.hypot(dx, dy);
    if (norm === 0) return { ...point };
    return { x: point.x - dy / norm * offset, y: point.y + dx / norm * offset };
  });
}
