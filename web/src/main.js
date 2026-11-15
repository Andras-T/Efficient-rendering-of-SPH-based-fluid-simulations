import './style.css';
import { applyObstaclePose, cloneMesh, identityPose } from './pose.js';
import { bindCamera, createCamera } from './camera.js';
import { colorMeshByPressure } from './cp.js';
import { createRenderer, requestGpu } from './gpu.js';
import { createGlRenderer, requestGl } from './gl.js';
import { createSolver, setInletSpeed, stepSolver } from './lbm.js';
import { fitMeshToTunnel, parsePackedMesh } from './mesh.js';
import { buildWindOccupancy } from './occupancy.js';
import { measureAero } from './aero.js';
import {
  Domain,
  clampWindKmh,
  DefaultWindKmh,
  kmhToLattice,
  latticeToKmh,
} from './units.js';

const canvas = document.querySelector('#view');
const present = document.querySelector('#present');
const fallback = document.querySelector('#fallback');
const fallbackText = document.querySelector('#fallback-text');
const statusEl = document.querySelector('#status');
const yawEl = document.querySelector('#yaw');
const pitchEl = document.querySelector('#pitch');
const windEl = document.querySelector('#wind');
const playEl = document.querySelector('#play');
const resetEl = document.querySelector('#reset');
const cdEl = document.querySelector('#cd');
const clEl = document.querySelector('#cl');
const kmhEl = document.querySelector('#kmh-label');
const yawLabel = document.querySelector('#yaw-label');
const pitchLabel = document.querySelector('#pitch-label');

function showFallback(reason) {
  fallback.classList.add('show');
  fallbackText.textContent = reason;
}

function resize() {
  const dpr = Math.min(window.devicePixelRatio || 1, 2);
  const width = Math.max(canvas.clientWidth, window.innerWidth, 1);
  const height = Math.max(canvas.clientHeight, window.innerHeight, 1);
  canvas.width = Math.floor(width * dpr);
  canvas.height = Math.floor(height * dpr);
}

async function chooseRenderer() {
  let gpu = { ok: false, reason: 'WebGPU unavailable.' };
  try {
    gpu = await requestGpu(canvas);
  } catch (error) {
    gpu = { ok: false, reason: error instanceof Error ? error.message : String(error) };
  }
  if (gpu.ok) {
    try {
      return { renderer: createRenderer(gpu, Domain), api: 'WebGPU', gpu };
    } catch (error) {
      gpu = { ok: false, reason: error instanceof Error ? error.message : String(error) };
    }
  }
  const gl = requestGl(canvas);
  if (gl.ok) {
    const renderer = createGlRenderer(gl, Domain);
    return { renderer, api: renderer.api, gpu: null };
  }
  return {
    renderer: null,
    api: null,
    gpu: null,
    reason: gl.reason || gpu.reason || 'WebGPU and WebGL are unavailable.',
  };
}

async function main() {
  resize();
  window.addEventListener('resize', resize);
  const chosen = await chooseRenderer();
  if (!chosen.renderer) {
    showFallback(chosen.reason);
    return;
  }
  const { renderer, api, gpu } = chosen;

  statusEl.textContent = 'Loading C42…';
  const packed = await fetch('./c42.bin');
  if (!packed.ok) {
    showFallback('Could not load the C42 mesh.');
    return;
  }
  const rest = parsePackedMesh(await packed.arrayBuffer());
  fitMeshToTunnel(rest, Domain);
  let live = cloneMesh(rest);
  let occupancy = buildWindOccupancy(Domain, live);
  let windKmh = DefaultWindKmh;
  let solver = createSolver(Domain, occupancy, 0.6, kmhToLattice(windKmh));
  colorMeshByPressure(live, Domain, solver.macros, occupancy, solver.inletUx);
  renderer.uploadMesh(live);
  const camera = createCamera();
  bindCamera(canvas, camera);

  let playing = false;
  let frames = 0;
  const syncLabels = () => {
    yawLabel.textContent = `${Math.round(Number(yawEl.value))}°`;
    pitchLabel.textContent = `${Math.round(Number(pitchEl.value))}°`;
    kmhEl.textContent = `${Math.round(windKmh)} km/h`;
  };
  yawEl.value = '0';
  pitchEl.value = '0';
  windEl.value = String(Math.round(DefaultWindKmh));
  syncLabels();

  const rebuild = () => {
    live = cloneMesh(rest);
    applyObstaclePose(
      live,
      {
        yaw: (Number(yawEl.value) * Math.PI) / 180,
        pitch: (Number(pitchEl.value) * Math.PI) / 180,
        roll: 0,
        translation: [0, 0, 0],
        scale: 1,
      },
      Domain,
    );
    occupancy = buildWindOccupancy(Domain, live);
    solver = createSolver(Domain, occupancy, 0.6, kmhToLattice(windKmh));
    colorMeshByPressure(live, Domain, solver.macros, occupancy, solver.inletUx);
    renderer.uploadMesh(live);
    frames = 0;
    statusEl.textContent = 'Occupancy rebuilt';
  };

  const applyPoseIfIdle = (event) => {
    if (event.type === 'pointerup' || event.type === 'change') {
      rebuild();
    }
  };
  for (const el of [yawEl, pitchEl]) {
    el.addEventListener('input', syncLabels);
    el.addEventListener('pointerup', applyPoseIfIdle);
    el.addEventListener('change', applyPoseIfIdle);
  }
  windEl.addEventListener('input', () => {
    windKmh = clampWindKmh(Number(windEl.value));
    setInletSpeed(solver, kmhToLattice(windKmh));
    syncLabels();
  });
  playEl.addEventListener('click', () => {
    playing = !playing;
    playEl.textContent = playing ? 'Pause' : 'Play';
  });
  resetEl.addEventListener('click', () => {
    yawEl.value = '0';
    pitchEl.value = '0';
    windEl.value = String(Math.round(DefaultWindKmh));
    windKmh = DefaultWindKmh;
    syncLabels();
    rebuild();
  });

  const usesGpu = Boolean(gpu);
  let blitPresent = usesGpu && /(?:\?|&)blit=1\b/.test(window.location.search);
  let blitBusy = false;
  const presentFrame = async () => {
    if (!blitPresent || blitBusy || !renderer.capture) {
      return;
    }
    blitBusy = true;
    try {
      const shot = await renderer.capture(camera, canvas.width, canvas.height);
      present.width = shot.width;
      present.height = shot.height;
      present.getContext('2d').putImageData(new ImageData(shot.rgba, shot.width, shot.height), 0, 0);
      present.classList.add('show');
    } finally {
      blitBusy = false;
    }
  };
  const tick = () => {
    resize();
    if (playing) {
      stepSolver(solver, 1);
      frames += 1;
      if (frames % 4 === 0) {
        colorMeshByPressure(live, Domain, solver.macros, occupancy, solver.inletUx);
        renderer.uploadMesh(live);
        const aero = measureAero(solver);
        cdEl.textContent = aero.cd.toFixed(2);
        clEl.textContent = aero.cl.toFixed(2);
        statusEl.textContent = `D3Q19  ${frames} steps  ${Math.round(latticeToKmh(solver.inletUx))} km/h`;
      }
    }
    renderer.draw(camera, canvas);
    if (blitPresent) {
      void presentFrame();
    }
    requestAnimationFrame(tick);
  };
  const adapterLabel = gpu?.adapter?.info?.description || gpu?.adapter?.info?.vendor || api;
  statusEl.textContent = `${adapterLabel}  D3Q19  C42`;
  window.__sphReady = true;
  window.__sphApi = api;
  window.__sphCapture = async () => renderer.capture(camera, canvas.width, canvas.height);
  requestAnimationFrame(tick);
  if (!usesGpu) {
    return;
  }
  (async () => {
    if (blitPresent) {
      return;
    }
    try {
      await new Promise((resolve) => requestAnimationFrame(resolve));
      const shot = await renderer.capture(camera, canvas.width, canvas.height);
      let gpuPainted = 0;
      for (let i = 0; i < shot.rgba.length; i += 32) {
        if (shot.rgba[i] > 8 || shot.rgba[i + 1] > 8 || shot.rgba[i + 2] > 8) {
          gpuPainted += 1;
        }
      }
      let swapPainted = 0;
      try {
        const bmp = await createImageBitmap(canvas);
        const probe = document.createElement('canvas');
        probe.width = bmp.width;
        probe.height = bmp.height;
        const ctx = probe.getContext('2d');
        ctx.drawImage(bmp, 0, 0);
        const data = ctx.getImageData(0, 0, bmp.width, bmp.height).data;
        for (let i = 0; i < data.length; i += 32) {
          if (data[i] > 8 || data[i + 1] > 8 || data[i + 2] > 8) {
            swapPainted += 1;
          }
        }
      } catch {
        swapPainted = 0;
      }
      if (gpuPainted > 8 && swapPainted < 8) {
        blitPresent = true;
        await presentFrame();
      }
    } catch (error) {
      console.error(error);
    }
  })();
}

main().catch((error) => {
  showFallback(error instanceof Error ? error.message : String(error));
});
