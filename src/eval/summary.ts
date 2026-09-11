import type { SimEvent } from '../core';

export interface RunSummary {
  readonly validation: 'not-yet-validated';
  readonly completed: number;
  readonly safetyClamps: number;
  readonly meanTravelTime: number | null;
  /** Completed trips only; excess over constant desired-speed travel, including source wait. */
  readonly meanDelay: number | null;
}

/** M0 diagnostic, not HCM control delay or a LOS evaluation. */
export function summarize(events: Iterable<SimEvent>): RunSummary {
  let completed = 0;
  let safetyClamps = 0;
  let totalTravelTime = 0;
  let totalDelay = 0;
  for (const event of events) {
    if (event.kind === 'safety-clamp') safetyClamps++;
    if (event.kind !== 'arrived') continue;
    completed++;
    totalTravelTime += event.travelTime;
    totalDelay += Math.max(0, event.travelTime + event.departureDelay - event.freeFlowTime);
  }
  return {
    validation: 'not-yet-validated', completed, safetyClamps,
    meanTravelTime: completed ? totalTravelTime / completed : null,
    meanDelay: completed ? totalDelay / completed : null,
  };
}
