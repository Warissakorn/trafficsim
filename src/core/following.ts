import type { DriverBehaviour, FollowingMode, VehicleType } from './types';

export interface Leader { readonly gap: number; readonly speed: number }
export interface FollowingResult { readonly acceleration: number; readonly mode: FollowingMode }

/**
 * Reduced four-regime, Wiedemann-inspired prototype. This is NOT W74 or W99.
 * Safety-distance shape follows W74; thresholds and acceleration laws are our own.
 * See docs/SIMULATION.md for equations, limitations and the source reference.
 */
export function followingAcceleration(
  speed: number, desiredSpeed: number, driverFactor: number,
  type: VehicleType, behaviour: DriverBehaviour, leader?: Leader,
): FollowingResult {
  const free = Math.min(type.maxAcceleration, (desiredSpeed - speed) / behaviour.followingTime);
  let acceleration = free;
  let mode: FollowingMode = 'free';
  if (leader) {
    const desiredGap = behaviour.standstillDistance +
      (behaviour.additiveSafetyDistance + behaviour.multiplicativeSafetyDistance * driverFactor) *
      Math.sqrt(Math.max(0, speed));
    const closing = speed - leader.speed;
    const room = Math.max(0.01, leader.gap - desiredGap);
    if (leader.gap < desiredGap) {
      mode = 'braking';
      acceleration = Math.min(free, -closing / behaviour.followingTime +
        (leader.gap - desiredGap) / behaviour.followingTime ** 2);
    } else if (closing > behaviour.speedThreshold &&
        room < closing ** 2 / (2 * type.comfortableDeceleration) + speed * behaviour.followingTime) {
      mode = 'approaching';
      acceleration = Math.min(free, -(closing ** 2) / (2 * room));
    } else if (leader.gap < desiredGap * 1.5) {
      mode = 'following';
      acceleration = Math.min(free, -closing / behaviour.followingTime +
        (leader.gap - desiredGap) / behaviour.followingTime ** 2);
    }
  }
  return { acceleration: Math.max(-type.maxDeceleration, acceleration), mode };
}

/** Ballistic displacement with a within-step stop; never integrates backwards. */
export function integrate(speed: number, acceleration: number, dt: number): { speed: number; distance: number } {
  if (acceleration < 0 && speed + acceleration * dt < 0) {
    return { speed: 0, distance: -(speed ** 2) / (2 * acceleration) };
  }
  return { speed: Math.max(0, speed + acceleration * dt), distance: Math.max(0, speed * dt + 0.5 * acceleration * dt ** 2) };
}
