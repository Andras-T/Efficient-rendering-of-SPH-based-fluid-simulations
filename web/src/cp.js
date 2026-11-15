import { WindFluid, WindInlet, WindOutlet, cellIndex } from './lbm.js';

export const AeroCpMin = -1.5;
export const AeroCpMax = 1.0;

export function colorFromCp(cp, cpMin = AeroCpMin, cpMax = AeroCpMax) {
  const blue = [0.04, 0.16, 0.93];
  const white = [0.93, 0.94, 0.96];
  const red = [0.94, 0.06, 0.04];
  const tone = (t) => Math.pow(Math.min(1, Math.max(0, t)), 0.52);
  const mix = (a, b, t) => [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t];
  if (cp >= 0) {
    const span = Math.max(cpMax, 1e-4);
    return mix(white, red, tone(cp / span));
  }
  const span = Math.min(cpMin, -1e-4);
  return mix(white, blue, tone(cp / span));
}

function fluidOccupancy(occ) {
  return occ === WindFluid || occ === WindInlet || occ === WindOutlet;
}

function sampleDensity(domain, macros, occupancy, point) {
  const gx = domain.grid[0];
  const gy = domain.grid[1];
  const gz = domain.grid[2];
  const coord = [
    (point[0] - domain.origin[0]) / domain.cellSize,
    (point[1] - domain.origin[1]) / domain.cellSize,
    (point[2] - domain.origin[2]) / domain.cellSize,
  ];
  const x0 = Math.floor(coord[0]);
  const y0 = Math.floor(coord[1]);
  const z0 = Math.floor(coord[2]);
  let sum = 0;
  let count = 0;
  for (let dz = -1; dz <= 1; ++dz) {
    for (let dy = -1; dy <= 1; ++dy) {
      for (let dx = -1; dx <= 1; ++dx) {
        const x = x0 + dx;
        const y = y0 + dy;
        const z = z0 + dz;
        if (x < 0 || y < 0 || z < 0 || x >= gx || y >= gy || z >= gz) {
          continue;
        }
        const index = cellIndex(x, y, z, domain.grid);
        if (!fluidOccupancy(occupancy[index])) {
          continue;
        }
        sum += macros[index * 4];
        count += 1;
      }
    }
  }
  return count === 0 ? -1 : sum / count;
}

export function colorMeshByPressure(mesh, domain, macros, occupancy, inletUx) {
  if (mesh.length < 3 || macros.length === 0) {
    return { cpMin: 0, cpMax: 0 };
  }
  const gy = domain.grid[1];
  const gz = domain.grid[2];
  let rhoInf = 0;
  let inletCount = 0;
  for (let z = 1; z + 1 < gz; ++z) {
    for (let y = 1; y + 1 < gy; ++y) {
      const index = 1 + y * domain.grid[0] + z * domain.grid[0] * gy;
      if (fluidOccupancy(occupancy[index])) {
        rhoInf += macros[index * 4];
        inletCount += 1;
      }
    }
  }
  rhoInf = inletCount === 0 ? 1 : rhoInf / inletCount;
  const dynamic = Math.max(1.5 * inletUx * inletUx, 1e-6);
  for (let i = 0; i + 2 < mesh.length; i += 3) {
    const a = mesh[i].position;
    const b = mesh[i + 1].position;
    const c = mesh[i + 2].position;
    const centroid = [(a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3, (a[2] + b[2] + c[2]) / 3];
    let nx = (b[1] - a[1]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[1] - a[1]);
    let ny = (b[2] - a[2]) * (c[0] - a[0]) - (b[0] - a[0]) * (c[2] - a[2]);
    let nz = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    const length = Math.hypot(nx, ny, nz);
    if (length > 1e-8) {
      nx /= length;
      ny /= length;
      nz /= length;
    } else {
      nx = 0;
      ny = 1;
      nz = 0;
    }
    const plus = [
      centroid[0] + nx * domain.cellSize * 0.55,
      centroid[1] + ny * domain.cellSize * 0.55,
      centroid[2] + nz * domain.cellSize * 0.55,
    ];
    const minus = [
      centroid[0] - nx * domain.cellSize * 0.55,
      centroid[1] - ny * domain.cellSize * 0.55,
      centroid[2] - nz * domain.cellSize * 0.55,
    ];
    let plusRho = sampleDensity(domain, macros, occupancy, plus);
    let minusRho = sampleDensity(domain, macros, occupancy, minus);
    if (plusRho < 0 && minusRho >= 0) {
      nx = -nx;
      ny = -ny;
      nz = -nz;
    }
    let density = minusRho >= 0 && plusRho < 0 ? minusRho : plusRho;
    if (density < 0) {
      density = minusRho;
    }
    if (density < 0) {
      for (let step = 2; step <= 6 && density < 0; ++step) {
        const offset = domain.cellSize * (0.45 * step);
        density = sampleDensity(domain, macros, occupancy, [
          centroid[0] + nx * offset,
          centroid[1] + ny * offset,
          centroid[2] + nz * offset,
        ]);
        if (density < 0) {
          density = sampleDensity(domain, macros, occupancy, [
            centroid[0] - nx * offset,
            centroid[1] - ny * offset,
            centroid[2] - nz * offset,
          ]);
        }
      }
    }
    if (density < 0) {
      density = rhoInf;
    }
    const lattice = (density - rhoInf) / dynamic;
    const live = Number.isFinite(lattice) ? Math.min(1, Math.max(-1.2, lattice)) : 0;
    const windward = Math.min(1, Math.max(0, -nx));
    const leeward = Math.min(1, Math.max(0, nx));
    const cp = 0.4 * live + 0.8 * windward - 0.55 * leeward;
    const color = colorFromCp(Number.isFinite(cp) ? cp : 0);
    mesh[i].color = color;
    mesh[i + 1].color = color;
    mesh[i + 2].color = color;
  }
  return { cpMin: AeroCpMin, cpMax: AeroCpMax };
}
