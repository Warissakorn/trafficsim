/** Engine contract. SI units throughout: metres, seconds, m/s, m/s². */
export interface Segment {
  readonly id: string;
  readonly length: number;
  readonly next: readonly string[];
}

export interface Route {
  readonly id: string;
  readonly segmentIds: readonly string[];
}

export interface DriverBehaviour {
  readonly id: string;
  readonly standstillDistance: number;
  readonly additiveSafetyDistance: number;
  readonly multiplicativeSafetyDistance: number;
  readonly followingTime: number;
  readonly speedThreshold: number;
}

export interface VehicleType {
  readonly id: string;
  readonly length: number;
  readonly width: number;
  readonly desiredSpeed: { readonly min: number; readonly max: number };
  readonly maxAcceleration: number;
  readonly comfortableDeceleration: number;
  readonly maxDeceleration: number;
  readonly behaviourId: string;
}

export interface VehicleInput {
  readonly id: string;
  readonly routeId: string;
  readonly vehicleTypeId: string;
  /** Poisson arrival rate, vehicles/hour, over [startTime, endTime). */
  readonly vehiclesPerHour: number;
  readonly startTime: number;
  readonly endTime: number;
}

export type SignalColor = 'red' | 'amber' | 'green';
export interface SignalProgram {
  readonly id: string;
  /** Cycle position at simulation time zero, in seconds. */
  readonly offset: number;
  readonly phases: readonly { readonly duration: number; readonly color: SignalColor }[];
}

export interface SignalHead {
  readonly id: string;
  readonly segmentId: string;
  /** Front-bumper stop-line coordinate, measured from segment start. */
  readonly position: number;
  readonly programId: string;
}

export interface Scenario {
  readonly duration: number;
  readonly timeStep: number;
  readonly segments: readonly Segment[];
  readonly routes: readonly Route[];
  readonly vehicleTypes: readonly VehicleType[];
  readonly behaviours: readonly DriverBehaviour[];
  readonly inputs: readonly VehicleInput[];
  readonly signalPrograms: readonly SignalProgram[];
  readonly signalHeads: readonly SignalHead[];
}

export type FollowingMode = 'free' | 'approaching' | 'following' | 'braking';
export interface PendingVehicle {
  readonly id: number;
  readonly inputId: string;
  readonly routeId: string;
  readonly vehicleTypeId: string;
  readonly scheduledTime: number;
  readonly desiredSpeed: number;
  readonly driverFactor: number;
}

export interface Vehicle extends PendingVehicle {
  readonly enteredTime: number;
  /** Front bumper distance along the entire route; never resets at a connector. */
  readonly distance: number;
  readonly speed: number;
  readonly acceleration: number;
  readonly mode: FollowingMode;
}

export interface InputState {
  readonly id: string;
  readonly nextArrival: number | null;
  readonly queue: readonly PendingVehicle[];
}

export type SimEvent =
  | { readonly kind: 'signal'; readonly time: number; readonly signalId: string; readonly color: SignalColor }
  | { readonly kind: 'departed'; readonly time: number; readonly vehicleId: number; readonly routeId: string; readonly scheduledTime: number; readonly desiredSpeed: number }
  | { readonly kind: 'moved'; readonly time: number; readonly vehicleId: number; readonly segmentId: string; readonly position: number; readonly speed: number; readonly acceleration: number }
  | { readonly kind: 'segment-entered'; readonly time: number; readonly vehicleId: number; readonly segmentId: string }
  | { readonly kind: 'safety-clamp'; readonly time: number; readonly vehicleId: number }
  | { readonly kind: 'arrived'; readonly time: number; readonly vehicleId: number; readonly routeId: string; readonly travelTime: number; readonly departureDelay: number; readonly freeFlowTime: number };

export interface SimState {
  readonly scenario: Scenario;
  readonly seed: number;
  readonly tick: number;
  readonly time: number;
  readonly randomState: number;
  readonly nextVehicleId: number;
  readonly inputs: readonly InputState[];
  readonly vehicles: readonly Vehicle[];
  readonly completed: number;
  /** Only events from the latest step; runSimulation streams the complete history. */
  readonly events: readonly SimEvent[];
}

export interface ValidationIssue { readonly code: string; readonly path: string }
export type EventStream = IterableIterator<SimEvent>;
export interface RunOptions { readonly includeMovementEvents?: boolean }
