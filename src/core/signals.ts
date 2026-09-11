import type { SignalColor, SignalProgram } from './types';

/** Phases are half-open [start, end); amber is a stop indication in M0. */
export function signalColorAt(program: SignalProgram, time: number): SignalColor {
  const cycle = program.phases.reduce((sum, phase) => sum + phase.duration, 0);
  let position = ((time + program.offset) % cycle + cycle) % cycle;
  if (cycle - position < 1e-9) position = 0;
  for (const phase of program.phases) {
    if (position < phase.duration - 1e-9) return phase.color;
    position -= phase.duration;
  }
  return program.phases[0]!.color;
}
