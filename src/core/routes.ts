import type { Route, Scenario, Vehicle } from './types';

export interface RoutePart { readonly segmentId: string; readonly start: number; readonly length: number }
export interface VehicleLocation { readonly segmentId: string; readonly position: number }
export interface OccupiedSpan { readonly vehicleId: number; readonly segmentId: string; readonly rear: number; readonly front: number; readonly speed: number }

export function routeParts(scenario: Scenario, route: Route): RoutePart[] {
  let start = 0;
  return route.segmentIds.map(segmentId => {
    const length = scenario.segments.find(segment => segment.id === segmentId)!.length;
    const part = { segmentId, start, length };
    start += length;
    return part;
  });
}

export function locateVehicle(scenario: Scenario, vehicle: Vehicle): VehicleLocation {
  const parts = routeParts(scenario, scenario.routes.find(route => route.id === vehicle.routeId)!);
  const part = parts.find(item => vehicle.distance < item.start + item.length) ?? parts.at(-1)!;
  return { segmentId: part.segmentId, position: Math.min(part.length, Math.max(0, vehicle.distance - part.start)) };
}

/** Keep a vehicle's rear on upstream segments while its front crosses a boundary. */
export function occupiedSpans(scenario: Scenario, vehicles: readonly Vehicle[]): OccupiedSpan[] {
  return vehicles.flatMap(vehicle => {
    const length = scenario.vehicleTypes.find(type => type.id === vehicle.vehicleTypeId)!.length;
    const parts = routeParts(scenario, scenario.routes.find(route => route.id === vehicle.routeId)!);
    return parts.filter(part => vehicle.distance >= part.start && vehicle.distance - length < part.start + part.length)
      .map(part => ({
        vehicleId: vehicle.id, segmentId: part.segmentId,
        rear: Math.max(0, vehicle.distance - length - part.start),
        front: Math.min(part.length, vehicle.distance - part.start), speed: vehicle.speed,
      }));
  });
}
