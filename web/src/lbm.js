export const WindFluid = 0;
export const WindSolid = 1;
export const WindInlet = 2;
export const WindOutlet = 3;
export const WindWall = 4;
export const LbmQ = 19;

export const LbmE = [
  [0, 0, 0],
  [1, 0, 0],
  [-1, 0, 0],
  [0, 1, 0],
  [0, -1, 0],
  [0, 0, 1],
  [0, 0, -1],
  [1, 1, 0],
  [-1, 1, 0],
  [1, -1, 0],
  [-1, -1, 0],
  [1, 0, 1],
  [-1, 0, 1],
  [1, 0, -1],
  [-1, 0, -1],
  [0, 1, 1],
  [0, -1, 1],
  [0, 1, -1],
  [0, -1, -1],
];

export const LbmW = [
  1 / 3,
  1 / 18, 1 / 18, 1 / 18, 1 / 18, 1 / 18, 1 / 18,
  1 / 36, 1 / 36, 1 / 36, 1 / 36, 1 / 36, 1 / 36, 1 / 36, 1 / 36, 1 / 36, 1 / 36, 1 / 36, 1 / 36,
];

export const LbmOpp = [0, 2, 1, 4, 3, 6, 5, 10, 9, 8, 7, 14, 13, 12, 11, 18, 17, 16, 15];

export function cellIndex(x, y, z, grid) {
  return x + y * grid[0] + z * grid[0] * grid[1];
}

export function lbmFeq(k, density, ux, uy, uz) {
  const e = LbmE[k];
  const eu = e[0] * ux + e[1] * uy + e[2] * uz;
  const uu = ux * ux + uy * uy + uz * uz;
  return LbmW[k] * density * (1 + 3 * eu + 4.5 * eu * eu - 1.5 * uu);
}

export function equilibriumPop(density, ux, uy, uz) {
  const f = new Float32Array(LbmQ);
  for (let k = 0; k < LbmQ; ++k) {
    f[k] = lbmFeq(k, density, ux, uy, uz);
  }
  return f;
}

export function createSolver(domain, occupancy, tau = 0.6, inletUx = 0.08) {
  const count = domain.grid[0] * domain.grid[1] * domain.grid[2];
  const current = new Float32Array(count * LbmQ);
  const scratch = new Float32Array(count * LbmQ);
  const macros = new Float32Array(count * 4);
  initPopulations(current, macros, occupancy, domain.grid, inletUx);
  scratch.set(current);
  return {
    domain,
    occupancy,
    tau,
    inletUx,
    current,
    scratch,
    macros,
    count,
  };
}

export function initPopulations(pops, macros, occupancy, grid, inletUx) {
  const count = grid[0] * grid[1] * grid[2];
  const rest = equilibriumPop(1, 0, 0, 0);
  const inlet = equilibriumPop(1, inletUx, 0, 0);
  for (let i = 0; i < count; ++i) {
    const occ = occupancy[i];
    const blocked = occ === WindSolid || occ === WindWall;
    const src = blocked ? rest : inlet;
    pops.set(src, i * LbmQ);
    macros[i * 4] = 1;
    macros[i * 4 + 1] = blocked ? 0 : inletUx;
    macros[i * 4 + 2] = 0;
    macros[i * 4 + 3] = 0;
  }
}

function collideInto(solver, src, dst) {
  const { occupancy, macros, tau, inletUx, count } = solver;
  const invTau = 1 / tau;
  for (let i = 0; i < count; ++i) {
    const occ = occupancy[i];
    const base = i * LbmQ;
    if (occ === WindSolid || occ === WindWall) {
      for (let k = 0; k < LbmQ; ++k) {
        dst[base + k] = src[base + k];
      }
      macros[i * 4] = 1;
      macros[i * 4 + 1] = 0;
      macros[i * 4 + 2] = 0;
      macros[i * 4 + 3] = 0;
      continue;
    }
    let density = 0;
    let mx = 0;
    let my = 0;
    let mz = 0;
    for (let k = 0; k < LbmQ; ++k) {
      const fk = src[base + k];
      density += fk;
      mx += fk * LbmE[k][0];
      my += fk * LbmE[k][1];
      mz += fk * LbmE[k][2];
    }
    density = Math.max(density, 1e-5);
    let ux = mx / density;
    let uy = my / density;
    let uz = mz / density;
    if (occ === WindInlet) {
      density = 1;
      ux = inletUx;
      uy = 0;
      uz = 0;
    }
    for (let k = 0; k < LbmQ; ++k) {
      const feq = lbmFeq(k, density, ux, uy, uz);
      dst[base + k] = src[base + k] + (feq - src[base + k]) * invTau;
    }
    macros[i * 4] = density;
    macros[i * 4 + 1] = ux;
    macros[i * 4 + 2] = uy;
    macros[i * 4 + 3] = uz;
  }
}

function streamInto(solver, src, dst) {
  const { occupancy, macros, inletUx, domain } = solver;
  const grid = domain.grid;
  const gx = grid[0];
  const gy = grid[1];
  const gz = grid[2];
  const inlet = equilibriumPop(1, inletUx, 0, 0);
  for (let z = 0; z < gz; ++z) {
    for (let y = 0; y < gy; ++y) {
      for (let x = 0; x < gx; ++x) {
        const i = cellIndex(x, y, z, grid);
        const occ = occupancy[i];
        const base = i * LbmQ;
        if (occ === WindSolid || occ === WindWall) {
          for (let k = 0; k < LbmQ; ++k) {
            dst[base + k] = src[base + k];
          }
          continue;
        }
        if (occ === WindInlet) {
          dst.set(inlet, base);
          macros[i * 4] = 1;
          macros[i * 4 + 1] = inletUx;
          macros[i * 4 + 2] = 0;
          macros[i * 4 + 3] = 0;
          continue;
        }
        if (occ === WindOutlet && x > 0) {
          const left = cellIndex(x - 1, y, z, grid) * LbmQ;
          for (let k = 0; k < LbmQ; ++k) {
            dst[base + k] = src[left + k];
          }
          continue;
        }
        for (let k = 0; k < LbmQ; ++k) {
          const sx = x - LbmE[k][0];
          const sy = y - LbmE[k][1];
          const sz = z - LbmE[k][2];
          if (sx < 0 || sy < 0 || sz < 0 || sx >= gx || sy >= gy || sz >= gz) {
            dst[base + k] = src[base + LbmOpp[k]];
            continue;
          }
          const srcIndex = cellIndex(sx, sy, sz, grid);
          const srcOcc = occupancy[srcIndex];
          if (srcOcc === WindSolid || srcOcc === WindWall) {
            dst[base + k] = src[base + LbmOpp[k]];
          } else {
            dst[base + k] = src[srcIndex * LbmQ + k];
          }
        }
      }
    }
  }
}

export function stepSolver(solver, steps = 1) {
  for (let s = 0; s < steps; ++s) {
    collideInto(solver, solver.current, solver.scratch);
    streamInto(solver, solver.scratch, solver.current);
  }
}

export function setInletSpeed(solver, latticeSpeed) {
  solver.inletUx = latticeSpeed;
}
