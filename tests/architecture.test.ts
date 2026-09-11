import { readdirSync, readFileSync } from 'node:fs';
import { resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { expect, it } from 'vitest';
import { coreBoundaryViolations } from '../tools/core-boundary';
import en from '../src/shell/locales/en.json';
import th from '../src/shell/locales/th.json';

const root = fileURLToPath(new URL('../src/core', import.meta.url));
it('keeps every core source free of external dependencies, wall clocks and unseeded RNG', () => {
  for (const file of readdirSync(root, { recursive: true }).map(String).filter(f => f.endsWith('.ts'))) {
    const path = resolve(root, file);
    expect(coreBoundaryViolations(readFileSync(path, 'utf8'), path, root), file).toEqual([]);
  }
});

it.each([
  'import type { Model } from "../model";',
  'export { App } from "../main";',
  'export * from "node:fs";',
  'const x = import("../../data/car.json");',
  'type T = import("react").ReactNode;',
  'import fs = require("node:fs");',
  'const mod = require("../model");',
  'const mod = import(variable);',
  'const now = Date.now();',
  'const draw = Math.random();',
])('demonstrates the core guard rejects a forbidden dependency: %s', source => {
  expect(coreBoundaryViolations(source, resolve(root, 'synthetic.ts'), root).length).toBeGreaterThan(0);
});

it('allows ordinary internal and type-only imports', () => {
  expect(coreBoundaryViolations('import type { Scenario } from "./types"; export { random } from "./random";', resolve(root, 'example.ts'), root)).toEqual([]);
});

it('provides matching nonempty English and Thai translation keys', () => {
  expect(Object.keys(th).sort()).toEqual(Object.keys(en).sort());
  for (const value of [...Object.values(en), ...Object.values(th)]) expect(value.trim().length).toBeGreaterThan(0);
});
