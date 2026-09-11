import { createSimulation, stepSimulation } from './core';
import type { SimEvent } from './core';
import { summarize } from './eval/summary';
import { createDemo } from './model/demo';
import { drawNetwork } from './render/network';
import en from './shell/locales/en.json';
import th from './shell/locales/th.json';
import './style.css';

const { network, scenario } = createDemo();
let state = createSimulation(scenario, 42);
let locale: 'en' | 'th' = 'en';
let running = false;
let frameId = 0;
let lastFrame = 0;
let accumulator = 0;
let events: SimEvent[] = [];
const app = document.querySelector<HTMLDivElement>('#app')!;
const t = (key: keyof typeof en) => ({ en, th }[locale][key]);
app.innerHTML = `
  <main>
    <header><div><h1 data-text="title"></h1><p data-text="subtitle"></p></div>
      <label><span data-text="language"></span><select id="language"><option value="en" data-text="english"></option><option value="th" data-text="thai"></option></select></label></header>
    <p class="notice" data-text="validation"></p>
    <p data-text="description"></p>
    <div class="controls">
      <label><span data-text="seed"></span><input id="seed" type="number" min="0" max="4294967295" step="1" value="42" /></label>
      <label><span data-text="speed"></span><select id="speed"><option value="1">1×</option><option value="5" selected>5×</option><option value="20">20×</option></select></label>
      <button id="run" data-text="run"></button><button id="step" data-text="step"></button><button id="reset" data-text="reset"></button>
    </div>
    <p id="error" role="alert"></p>
    <canvas></canvas>
    <div class="metrics">${['time', 'active', 'pending', 'completed', 'delay'].map(key => `<div><span data-text="${key}"></span><strong id="${key}"></strong></div>`).join('')}</div>
    <p class="scope" data-text="scope"></p>
  </main>`;
const canvas = app.querySelector<HTMLCanvasElement>('canvas')!;
const run = app.querySelector<HTMLButtonElement>('#run')!;
const step = app.querySelector<HTMLButtonElement>('#step')!;
const seedInput = app.querySelector<HTMLInputElement>('#seed')!;
const speedInput = app.querySelector<HTMLSelectElement>('#speed')!;
const error = app.querySelector<HTMLParagraphElement>('#error')!;

function refresh() {
  for (const element of app.querySelectorAll<HTMLElement>('[data-text]')) {
    element.textContent = t(element.dataset.text as keyof typeof en);
  }
  document.documentElement.lang = locale;
  canvas.setAttribute('aria-label', t('canvas'));
  run.textContent = t(running ? 'pause' : 'run');
  step.disabled = running || state.time >= scenario.duration || Boolean(error.textContent);
  run.disabled = (!running && state.time >= scenario.duration) || Boolean(error.textContent);
  seedInput.disabled = running;
  const summary = summarize(events);
  const values = {
    time: state.time.toFixed(1), active: state.vehicles.length,
    pending: state.inputs.reduce((sum, input) => sum + input.queue.length, 0),
    completed: state.completed, delay: summary.meanDelay?.toFixed(1) ?? t('empty'),
  };
  for (const [id, value] of Object.entries(values)) app.querySelector(`#${id}`)!.textContent = String(value);
  if (error.textContent) error.textContent = t('invalidSeed');
  drawNetwork(canvas, network, state);
}

function advance() {
  state = stepSimulation(state);
  events.push(...state.events.filter(event => event.kind !== 'moved'));
}

function stop() {
  running = false;
  cancelAnimationFrame(frameId);
  accumulator = 0;
  refresh();
}

function frame(now: number) {
  if (!running) return;
  accumulator += Math.min(0.25, (now - lastFrame) / 1000) * Number(speedInput.value);
  lastFrame = now;
  while (accumulator >= scenario.timeStep && state.time < scenario.duration) {
    advance();
    accumulator -= scenario.timeStep;
  }
  if (state.time >= scenario.duration) { stop(); return; }
  refresh();
  frameId = requestAnimationFrame(frame);
}

run.addEventListener('click', () => {
  if (running) { stop(); return; }
  running = true;
  lastFrame = performance.now();
  refresh();
  frameId = requestAnimationFrame(frame);
});
step.addEventListener('click', () => { advance(); refresh(); });
function reset() {
  const seed = Number(seedInput.value);
  if (!seedInput.value.trim() || !Number.isSafeInteger(seed) || seed < 0 || seed > 0xffffffff) {
    error.textContent = t('invalidSeed'); refresh(); return;
  }
  stop();
  state = createSimulation(scenario, seed);
  events = [];
  error.textContent = '';
  refresh();
}
app.querySelector('#reset')!.addEventListener('click', reset);
seedInput.addEventListener('change', reset);
app.querySelector('#language')!.addEventListener('change', event => {
  locale = (event.target as HTMLSelectElement).value as typeof locale;
  refresh();
});
new ResizeObserver(refresh).observe(canvas);
refresh();
