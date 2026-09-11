# Tools guide

Developer and command-line utilities for TrafficSim. Run every command in this guide from
the repository root unless a section says otherwise.

## Setup

TrafficSim requires Node.js 22.12 or newer. Python 3 is required only for the file-size
checker.

```bash
npm ci
```

Use `npm ci` for a clean, reproducible installation from `package-lock.json`. Use
`npm install` only when deliberately changing dependencies.

## Quick reference

| Tool | Purpose | Normal command | Success exit code |
|---|---|---|---|
| `run-simulation.ts` | Run the built-in crossing scenario without the UI | `npm run simulate -- 42` | `0` |
| `core-boundary.ts` | Detect forbidden dependencies and nondeterministic APIs in `src/core/` | `npm test -- tests/architecture.test.ts` | `0` |
| `check_file_sizes.py` | Report source files and fail when one exceeds the line budget | `python3 tools/check_file_sizes.py .` | `0` |

Run the complete project check before committing:

```bash
npm run check
```

This runs all tests, the strict TypeScript and production build, and the default 500-line
source-file check.

## Headless simulation runner

File: `tools/run-simulation.ts`

The runner loads the built-in network and scenario from `data/scenarios/crossing.json`,
creates an immutable simulation with the selected random seed, advances it to the scenario
horizon, and prints a JSON summary. It is useful for smoke tests and reproducibility checks;
it is not a general project-file runner yet.

### Usage

```bash
npm run simulate
npm run simulate -- 42
npm run simulate -- 123456
```

The optional positional argument is the random seed. The default is `42`. Valid seeds are
integers from `0` to `4294967295` inclusive. The same scenario, seed, engine version and
JavaScript runtime must reproduce the same event stream.

Do not pass options before the `--`; npm would interpret them as npm options rather than
arguments for the runner.

### Output

```json
{
  "seed": 42,
  "simulatedSeconds": 180,
  "validation": "not-yet-validated",
  "completed": 31,
  "safetyClamps": 0,
  "meanTravelTime": 51.961290322580645,
  "meanDelay": 29.249359418430977,
  "active": 0,
  "pending": 0
}
```

| Field | Meaning |
|---|---|
| `seed` | Seed used by the xorshift32 random-number generator |
| `simulatedSeconds` | Scenario time reached by fixed stepping |
| `validation` | Permanent warning for the current unvalidated model |
| `completed` | Vehicles that reached the end of their route |
| `safetyClamps` | Steps where overlap prevention shortened a proposed movement |
| `meanTravelTime` | Mean network travel time for completed trips, in seconds |
| `meanDelay` | Completed-trip diagnostic delay, in seconds |
| `active` | Vehicles still inside the network at the horizon |
| `pending` | Generated vehicles still waiting to enter at the horizon |

`meanDelay` includes source waiting and acceleration loss. It is not HCM control delay,
movement delay or LOS. It uses completed trips only, so always inspect `active` and
`pending` before interpreting it. `safetyClamps` should be investigated when nonzero; the
guard prevents overlap but may indicate an aggressive timestep, parameter set or topology.

To run a different built-in scenario, change `createDemo()` in `src/model/demo.ts` or add a
new explicit runner. Do not make this tool parse persisted project files; parsing belongs in
the future `src/project/` layer.

### Common errors

| Error | Cause and action |
|---|---|
| `Seed must be an unsigned 32-bit integer` | Pass one integer in the allowed range after `--` |
| `ScenarioValidationError` | The compiled demo contains an invalid route, parameter, timing or unsupported topology; read every reported `code: path` pair |
| `ERR_MODULE_NOT_FOUND` | Dependencies are missing; run `npm ci` from the repository root |
| Different result for seed 42 | Do not accept silently; run the tests and review the trajectory-fingerprint change |

## Core boundary checker

File: `tools/core-boundary.ts`

The checker protects the most important architecture rule: `src/core/` may import only
other files inside `src/core/`. It parses TypeScript syntax with the compiler AST rather
than matching text, so it detects ordinary imports, type-only imports, re-exports,
`require`, import types and dynamic imports.

It also rejects runtime APIs that would make simulation results depend on external state:

- Package, Node.js, browser, model, UI and I/O imports
- Triple-slash references and external type directives
- Non-literal `require()` or `import()` calls
- `Date`, `performance`, `fetch`, `XMLHttpRequest` and `WebSocket`
- `setTimeout`, `setInterval` and unseeded `Math.random()`

### Usage

The file exports a function and is intentionally exercised through the architecture test:

```bash
npm test -- tests/architecture.test.ts
```

To run the entire suite that includes this guard:

```bash
npm test
```

The test scans all TypeScript files recursively under `src/core/`. It also feeds deliberate
bad-import examples to the checker, proving the guard fails for the cases it claims to
block. Do not replace this with a test that merely checks the current files are clean.

### Reading failures

Typical messages are:

```text
External core dependency: ../model
External core dependency: node:fs
External core type dependency: node
Non-literal core import
Forbidden core runtime: Date
Unseeded Math.random
```

Move the dependency out of `src/core/` and pass plain scenario data or consume emitted
events at the boundary. Do not add an exception for convenience. If a dependency is
genuinely necessary, change the architecture decision and its reasoning before changing
the checker.

The checker currently uses the TypeScript 5.9.3 public compiler API. Keep that version
pinned until the guard is migrated and its negative fixtures pass against the replacement.

## Source-file size checker

File: `tools/check_file_sizes.py`

The checker recursively counts lines in recognized source files, prints the largest files,
and returns a failing exit code when any file is over the configured budget. It skips
generated, dependency, cache and build directories such as `node_modules`, `dist`,
`.vite`, `.git`, `vendor` and `target`.

Recognized extensions include TypeScript, JavaScript, Python, C/C++, C#, Rust, Go, Java,
Kotlin, Ruby, Lua and Swift source files. Markdown, JSON, CSS and HTML are not counted.

### Usage

```bash
# Check the current repository with the default 500-line limit.
python3 tools/check_file_sizes.py .

# Show only the ten largest source files.
python3 tools/check_file_sizes.py . --top 10

# Test a temporary 300-line budget.
python3 tools/check_file_sizes.py . --limit 300

# Inspect another directory.
python3 tools/check_file_sizes.py ../another-project --top 30
```

Arguments:

| Argument | Default | Meaning |
|---|---:|---|
| `target` | `.` | Directory scanned recursively |
| `--limit` | `500` | Maximum allowed lines per recognized source file |
| `--top` | `20` | Number of largest files printed |

Exit code `0` means no recognized source file exceeds the limit. Exit code `1` means at
least one file is over the limit. A directory with no recognized source files also returns
`0` and prints a notice.

When a file is too large, split it along a real responsibility boundary—such as data versus
logic, simulation versus rendering, or validation versus compilation. Do not divide a file
at an arbitrary line merely to satisfy the checker. Update `docs/ARCHITECTURE.md` if the
split changes system ownership.

## Recommended workflows

### Before a commit

```bash
npm run check
npm run simulate -- 42
```

Confirm the tests and build pass, all source files remain within budget, and the reference
run still makes sense. A changed seed-42 result may be intentional, but it requires review
because the test suite stores its trajectory fingerprint.

### After changing `src/core/`

```bash
npm test -- tests/architecture.test.ts tests/simulation.test.ts tests/validation.test.ts
npm run simulate -- 42
```

Check boundary enforcement, deterministic dynamics, invalid-input handling and the
headless result together.

### After changing the network model

```bash
npm test -- tests/network.test.ts tests/simulation.test.ts
npm run simulate -- 42
```

This verifies authoring geometry and compilation, then exercises the resulting runtime
scenario end to end.

## Tool ownership rules

- Tools may orchestrate the application and print to standard output; `src/core/` may not.
- A reusable simulation algorithm belongs in `src/core/`, not in a script under `tools/`.
- Network authoring and compilation belong in `src/model/network/`.
- Persisted-file parsing, migration and validation will belong in `src/project/`.
- Tests must exercise a tool's real invocation path, not a second implementation of it.
- Keep output machine-readable when a tool may later be used in CI or batch automation.
