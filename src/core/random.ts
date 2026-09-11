/** xorshift32 with explicit state; the returned variate is strictly between 0 and 1. */
export function random(state: number): { state: number; value: number } {
  let next = state | 0;
  next ^= next << 13;
  next ^= next >>> 17;
  next ^= next << 5;
  next >>>= 0;
  return { state: next, value: (next + 0.5) / 4294967296 };
}

export function normalizeSeed(seed: number): number {
  if (!Number.isSafeInteger(seed) || seed < 0 || seed > 0xffffffff) {
    throw new RangeError('Seed must be an unsigned 32-bit integer');
  }
  return seed === 0 ? 0x6d2b79f5 : seed;
}
