import { WindFluid, WindSolid, WindInlet, WindOutlet, WindWall, cellIndex } from './lbm.js';

function distPointTriangleSq(p, a, b, c) {
  const ab = [b[0] - a[0], b[1] - a[1], b[2] - a[2]];
  const ac = [c[0] - a[0], c[1] - a[1], c[2] - a[2]];
  const ap = [p[0] - a[0], p[1] - a[1], p[2] - a[2]];
  const d1 = ab[0] * ap[0] + ab[1] * ap[1] + ab[2] * ap[2];
  const d2 = ac[0] * ap[0] + ac[1] * ap[1] + ac[2] * ap[2];
  if (d1 <= 0 && d2 <= 0) {
    return ap[0] * ap[0] + ap[1] * ap[1] + ap[2] * ap[2];
  }
  const bp = [p[0] - b[0], p[1] - b[1], p[2] - b[2]];
  const d3 = ab[0] * bp[0] + ab[1] * bp[1] + ab[2] * bp[2];
  const d4 = ac[0] * bp[0] + ac[1] * bp[1] + ac[2] * bp[2];
  if (d3 >= 0 && d4 <= d3) {
    return bp[0] * bp[0] + bp[1] * bp[1] + bp[2] * bp[2];
  }
  const vc = d1 * d4 - d3 * d2;
  if (vc <= 0 && d1 >= 0 && d3 <= 0) {
    const v = d1 / (d1 - d3);
    const dx = p[0] - (a[0] + v * ab[0]);
    const dy = p[1] - (a[1] + v * ab[1]);
    const dz = p[2] - (a[2] + v * ab[2]);
    return dx * dx + dy * dy + dz * dz;
  }
  const cp = [p[0] - c[0], p[1] - c[1], p[2] - c[2]];
  const d5 = ab[0] * cp[0] + ab[1] * cp[1] + ab[2] * cp[2];
  const d6 = ac[0] * cp[0] + ac[1] * cp[1] + ac[2] * cp[2];
  if (d6 >= 0 && d5 <= d6) {
    return cp[0] * cp[0] + cp[1] * cp[1] + cp[2] * cp[2];
  }
  const vb = d5 * d2 - d1 * d6;
  if (vb <= 0 && d2 >= 0 && d6 <= 0) {
    const w = d2 / (d2 - d6);
    const dx = p[0] - (a[0] + w * ac[0]);
    const dy = p[1] - (a[1] + w * ac[1]);
    const dz = p[2] - (a[2] + w * ac[2]);
    return dx * dx + dy * dy + dz * dz;
  }
  const va = d3 * d6 - d5 * d4;
  if (va <= 0 && d4 - d3 >= 0 && d5 - d6 >= 0) {
    const w = (d4 - d3) / (d4 - d3 + (d5 - d6));
    const dx = p[0] - (b[0] + w * (c[0] - b[0]));
    const dy = p[1] - (b[1] + w * (c[1] - b[1]));
    const dz = p[2] - (b[2] + w * (c[2] - b[2]));
    return dx * dx + dy * dy + dz * dz;
  }
  const denom = 1 / (va + vb + vc);
  const v = vb * denom;
  const w = vc * denom;
  const dx = p[0] - (a[0] + ab[0] * v + ac[0] * w);
  const dy = p[1] - (a[1] + ab[1] * v + ac[1] * w);
  const dz = p[2] - (a[2] + ab[2] * v + ac[2] * w);
  return dx * dx + dy * dy + dz * dz;
}

export function buildWindOccupancy(domain, mesh) {
  const [gx, gy, gz] = domain.grid;
  const count = gx * gy * gz;
  const occupancy = new Uint8Array(count);
  for (let z = 0; z < gz; ++z) {
    for (let y = 0; y < gy; ++y) {
      for (let x = 0; x < gx; ++x) {
        const index = cellIndex(x, y, z, domain.grid);
        if (x === 0) {
          occupancy[index] = WindInlet;
        } else if (x + 1 === gx) {
          occupancy[index] = WindOutlet;
        } else if (y === 0 || y + 1 === gy || z === 0 || z + 1 === gz) {
          occupancy[index] = WindWall;
        } else {
          occupancy[index] = WindFluid;
        }
      }
    }
  }

  const reach = domain.cellSize * 0.78;
  const reachSq = reach * reach;
  for (let i = 0; i + 2 < mesh.length; i += 3) {
    const a = mesh[i].position;
    const b = mesh[i + 1].position;
    const c = mesh[i + 2].position;
    const tmin = [
      Math.min(a[0], b[0], c[0]),
      Math.min(a[1], b[1], c[1]),
      Math.min(a[2], b[2], c[2]),
    ];
    const tmax = [
      Math.max(a[0], b[0], c[0]),
      Math.max(a[1], b[1], c[1]),
      Math.max(a[2], b[2], c[2]),
    ];
    const c0 = [0, 0, 0];
    const c1 = [0, 0, 0];
    for (let axis = 0; axis < 3; ++axis) {
      c0[axis] = Math.max(
        0,
        Math.min(
          domain.grid[axis] - 1,
          Math.floor((tmin[axis] - reach - domain.origin[axis]) / domain.cellSize),
        ),
      );
      c1[axis] = Math.max(
        0,
        Math.min(
          domain.grid[axis] - 1,
          Math.floor((tmax[axis] + reach - domain.origin[axis]) / domain.cellSize),
        ),
      );
    }
    for (let z = c0[2]; z <= c1[2]; ++z) {
      for (let y = c0[1]; y <= c1[1]; ++y) {
        for (let x = c0[0]; x <= c1[0]; ++x) {
          const index = cellIndex(x, y, z, domain.grid);
          if (occupancy[index] !== WindFluid) {
            continue;
          }
          const point = [
            domain.origin[0] + domain.cellSize * (x + 0.5),
            domain.origin[1] + domain.cellSize * (y + 0.5),
            domain.origin[2] + domain.cellSize * (z + 0.5),
          ];
          if (distPointTriangleSq(point, a, b, c) <= reachSq) {
            occupancy[index] = WindSolid;
          }
        }
      }
    }
  }

  const reached = new Uint8Array(count);
  const queue = [];
  const tryEnqueue = (x, y, z) => {
    if (x < 0 || y < 0 || z < 0 || x >= gx || y >= gy || z >= gz) {
      return;
    }
    const index = cellIndex(x, y, z, domain.grid);
    if (reached[index] !== 0 || occupancy[index] !== WindFluid) {
      return;
    }
    reached[index] = 1;
    queue.push(index);
  };
  for (let z = 1; z + 1 < gz; ++z) {
    for (let y = 1; y + 1 < gy; ++y) {
      tryEnqueue(1, y, z);
    }
  }
  const dx = [1, -1, 0, 0, 0, 0];
  const dy = [0, 0, 1, -1, 0, 0];
  const dz = [0, 0, 0, 0, 1, -1];
  let head = 0;
  while (head < queue.length) {
    const index = queue[head++];
    const x = index % gx;
    const y = Math.floor(index / gx) % gy;
    const z = Math.floor(index / (gx * gy));
    for (let n = 0; n < 6; ++n) {
      tryEnqueue(x + dx[n], y + dy[n], z + dz[n]);
    }
  }
  for (let i = 0; i < count; ++i) {
    if (occupancy[i] === WindFluid && reached[i] === 0) {
      occupancy[i] = WindSolid;
    }
  }
  return occupancy;
}

export function frontalCells(domain, occupancy) {
  const [gx, gy, gz] = domain.grid;
  const seen = new Uint8Array(gy * gz);
  let cells = 0;
  for (let z = 0; z < gz; ++z) {
    for (let y = 0; y < gy; ++y) {
      for (let x = 0; x < gx; ++x) {
        if (occupancy[cellIndex(x, y, z, domain.grid)] !== WindSolid) {
          continue;
        }
        const silhouette = y + z * gy;
        if (seen[silhouette] !== 0) {
          continue;
        }
        seen[silhouette] = 1;
        cells += 1;
      }
    }
  }
  return cells;
}
