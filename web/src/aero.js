import { WindFluid, WindInlet, WindOutlet, WindSolid, LbmE, LbmOpp, LbmW, LbmQ, cellIndex } from './lbm.js';
import { frontalCells } from './occupancy.js';

export function measureAero(solver) {
  const grid = solver.domain.grid;
  const gx = grid[0];
  const gy = grid[1];
  const gz = grid[2];
  const { occupancy, current, inletUx } = solver;
  let fx = 0;
  let fy = 0;
  let fz = 0;
  for (let z = 0; z < gz; ++z) {
    for (let y = 0; y < gy; ++y) {
      for (let x = 0; x < gx; ++x) {
        const index = cellIndex(x, y, z, grid);
        const occ = occupancy[index];
        if (occ !== WindFluid && occ !== WindInlet && occ !== WindOutlet) {
          continue;
        }
        const base = index * LbmQ;
        for (let k = 1; k < LbmQ; ++k) {
          const nx = x + LbmE[k][0];
          const ny = y + LbmE[k][1];
          const nz = z + LbmE[k][2];
          if (nx < 0 || ny < 0 || nz < 0 || nx >= gx || ny >= gy || nz >= gz) {
            continue;
          }
          const neighbor = occupancy[cellIndex(nx, ny, nz, grid)];
          if (neighbor !== WindSolid) {
            continue;
          }
          const bounced = LbmOpp[k];
          const fi = current[base + bounced];
          const force = 2 * (fi - LbmW[k]);
          fx += force * LbmE[k][0];
          fy += force * LbmE[k][1];
          fz += force * LbmE[k][2];
        }
      }
    }
  }
  const area = Math.max(frontalCells(solver.domain, occupancy), 1);
  const q = 0.5 * inletUx * inletUx * area;
  const cd = fx / q;
  const cl = fy / q;
  return {
    drag: fx,
    lift: fy,
    side: fz,
    cd: Number.isFinite(cd) ? cd : 0,
    cl: Number.isFinite(cl) ? cl : 0,
    frontalArea: area * solver.domain.cellSize * solver.domain.cellSize,
  };
}
