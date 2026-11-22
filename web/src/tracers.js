import { WindFluid, WindInlet, WindOutlet, WindSolid, WindWall, cellIndex } from './lbm.js';
import { Domain } from './units.js';

export const TracerInletY = 14;
export const TracerInletZ = 18;
export const TracerLayers = 4;
export const TracerCount = TracerInletY * TracerInletZ * TracerLayers;

function hash(x) {
  x ^= x >>> 16;
  x = Math.imul(x, 0x7feb352d);
  x ^= x >>> 15;
  x = Math.imul(x, 0x846ca68b);
  x ^= x >>> 16;
  return x >>> 0;
}

function fluid(occ) {
  return occ === WindFluid || occ === WindInlet || occ === WindOutlet;
}

function occupancyAt(solver, point) {
  const { domain, occupancy } = solver;
  const gx = domain.grid[0];
  const gy = domain.grid[1];
  const gz = domain.grid[2];
  const x = Math.floor((point[0] - domain.origin[0]) / domain.cellSize);
  const y = Math.floor((point[1] - domain.origin[1]) / domain.cellSize);
  const z = Math.floor((point[2] - domain.origin[2]) / domain.cellSize);
  if (x < 0 || y < 0 || z < 0 || x >= gx || y >= gy || z >= gz) {
    return WindWall;
  }
  return occupancy[cellIndex(x, y, z, domain.grid)];
}

export function sampleVelocity(solver, point) {
  const { domain, macros, occupancy } = solver;
  const gx = domain.grid[0];
  const gy = domain.grid[1];
  const gz = domain.grid[2];
  const fx = (point[0] - domain.origin[0]) / domain.cellSize;
  const fy = (point[1] - domain.origin[1]) / domain.cellSize;
  const fz = (point[2] - domain.origin[2]) / domain.cellSize;
  const x0 = Math.floor(fx);
  const y0 = Math.floor(fy);
  const z0 = Math.floor(fz);
  let ux = 0;
  let uy = 0;
  let uz = 0;
  let wsum = 0;
  for (let dz = 0; dz <= 1; ++dz) {
    for (let dy = 0; dy <= 1; ++dy) {
      for (let dx = 0; dx <= 1; ++dx) {
        const x = x0 + dx;
        const y = y0 + dy;
        const z = z0 + dz;
        if (x < 0 || y < 0 || z < 0 || x >= gx || y >= gy || z >= gz) {
          continue;
        }
        const index = cellIndex(x, y, z, domain.grid);
        if (!fluid(occupancy[index])) {
          continue;
        }
        const wx = dx ? fx - x0 : 1 - (fx - x0);
        const wy = dy ? fy - y0 : 1 - (fy - y0);
        const wz = dz ? fz - z0 : 1 - (fz - z0);
        const w = Math.max(wx, 0) * Math.max(wy, 0) * Math.max(wz, 0);
        const base = index * 4;
        ux += macros[base + 1] * w;
        uy += macros[base + 2] * w;
        uz += macros[base + 3] * w;
        wsum += w;
      }
    }
  }
  if (wsum < 1e-6) {
    return [solver.inletUx, 0, 0];
  }
  return [ux / wsum, uy / wsum, uz / wsum];
}

function seedOne(i, solver, domain) {
  let state = hash(i * 747796405 + 18);
  const rand01 = () => {
    state = hash(state);
    return state / 4294967296;
  };
  const rake = i % (TracerInletY * TracerInletZ);
  const layer = Math.floor(i / (TracerInletY * TracerInletZ));
  const iy = rake % TracerInletY;
  const iz = Math.floor(rake / TracerInletY);
  const y01 = Math.min(1, Math.max(0, (iy + 0.2 + 0.6 * rand01()) / TracerInletY));
  const z01 = Math.min(1, Math.max(0, (iz + 0.2 + 0.6 * rand01()) / TracerInletZ));
  const extentY = domain.cellSize * domain.grid[1];
  const extentZ = domain.cellSize * domain.grid[2];
  const extentX = domain.cellSize * domain.grid[0];
  const y = domain.origin[1] + domain.cellSize * 1.2 + y01 * (extentY - domain.cellSize * 2.4);
  const z = domain.origin[2] + domain.cellSize * 1.2 + z01 * (extentZ - domain.cellSize * 2.4);
  const x =
    domain.origin[0] +
    domain.cellSize * 1.6 +
    layer * (0.22 * extentX) +
    0.08 * extentX * rand01();
  const vel = [solver.inletUx, 0, 0];
  return {
    position: [x, y, z],
    prev: [x - domain.cellSize * 2.4, y, z],
    velocity: vel,
  };
}

export function createTracers(solver, domain = Domain) {
  const tracers = new Array(TracerCount);
  for (let i = 0; i < TracerCount; ++i) {
    tracers[i] = seedOne(i, solver, domain);
  }
  return tracers;
}

export function stepTracers(tracers, solver, steps = 1, domain = Domain) {
  const cell = domain.cellSize;
  const xMax = domain.origin[0] + cell * domain.grid[0] - cell * 1.4;
  const xMin = domain.origin[0] + cell * 1.2;
  for (let s = 0; s < steps; ++s) {
    for (let i = 0; i < tracers.length; ++i) {
      const tr = tracers[i];
      const vel = sampleVelocity(solver, tr.position);
      tr.velocity = vel;
      const next = [
        tr.position[0] + vel[0] * cell * 2.4,
        tr.position[1] + vel[1] * cell * 2.4,
        tr.position[2] + vel[2] * cell * 2.4,
      ];
      const occ = occupancyAt(solver, next);
      if (occ === WindSolid || occ === WindWall || next[0] > xMax || next[0] < xMin) {
        tracers[i] = seedOne(i, solver, domain);
        tracers[i].position[0] = xMin + cell * 0.4;
        tracers[i].prev = [
          tracers[i].position[0] - cell * 2.4,
          tracers[i].position[1],
          tracers[i].position[2],
        ];
        continue;
      }
      tr.prev = tr.position.slice();
      tr.position = next;
    }
  }
}

function speedColor(speed, inletUx) {
  const t = Math.min(1, Math.max(0, speed / Math.max(Math.abs(inletUx) * 1.15, 0.02)));
  return [
    0.12 + 0.86 * t,
    0.62 + 0.32 * t,
    1.0 - 0.72 * t,
  ];
}

export function packStreaks(tracers, inletUx, halfWidth = 0.012) {
  const data = new Float32Array(tracers.length * 6 * 6);
  let o = 0;
  const emit = (p, color) => {
    data[o] = p[0];
    data[o + 1] = p[1];
    data[o + 2] = p[2];
    data[o + 3] = color[0];
    data[o + 4] = color[1];
    data[o + 5] = color[2];
    o += 6;
  };
  for (const tr of tracers) {
    const dx = tr.position[0] - tr.prev[0];
    const dy = tr.position[1] - tr.prev[1];
    const dz = tr.position[2] - tr.prev[2];
    let len = Math.hypot(dx, dy, dz);
    if (len < 1e-5) {
      len = 1;
    }
    const along = [dx / len, dy / len, dz / len];
    const side = Math.abs(along[1]) < 0.9 ? [0, 1, 0] : [1, 0, 0];
    let sx = along[1] * side[2] - along[2] * side[1];
    let sy = along[2] * side[0] - along[0] * side[2];
    let sz = along[0] * side[1] - along[1] * side[0];
    const sl = Math.hypot(sx, sy, sz) || 1;
    sx = (sx / sl) * halfWidth;
    sy = (sy / sl) * halfWidth;
    sz = (sz / sl) * halfWidth;
    const tailScale = Math.max(0.055, Math.min(0.11, len * 3.2));
    const tail = [
      tr.position[0] - along[0] * tailScale,
      tr.position[1] - along[1] * tailScale,
      tr.position[2] - along[2] * tailScale,
    ];
    const speed = Math.hypot(tr.velocity[0], tr.velocity[1], tr.velocity[2]);
    const color = speedColor(speed, inletUx);
    const a = [tail[0] - sx, tail[1] - sy, tail[2] - sz];
    const b = [tail[0] + sx, tail[1] + sy, tail[2] + sz];
    const c = [tr.position[0] - sx, tr.position[1] - sy, tr.position[2] - sz];
    const d = [tr.position[0] + sx, tr.position[1] + sy, tr.position[2] + sz];
    emit(a, color);
    emit(b, color);
    emit(c, color);
    emit(b, color);
    emit(d, color);
    emit(c, color);
  }
  return data;
}
