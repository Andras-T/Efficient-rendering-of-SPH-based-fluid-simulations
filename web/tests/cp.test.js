import assert from 'node:assert/strict';
import test from 'node:test';
import { Domain } from '../src/units.js';
import { colorFromCp, colorMeshByPressure } from '../src/cp.js';
import { applyObstaclePose, cloneMesh } from '../src/pose.js';
import { WindFluid, WindInlet, WindOutlet, WindWall } from '../src/lbm.js';

function box() {
  const mesh = [];
  const face = (a, b, c, d, n) => {
    for (const p of [a, b, c, a, c, d]) {
      mesh.push({ position: p.slice(), normal: n.slice(), color: [0.5, 0.5, 0.5] });
    }
  };
  const mid = Domain.origin[0] + 0.5 * Domain.cellSize * Domain.grid[0];
  const y0 = Domain.origin[1] + 0.3;
  face(
    [mid - 0.2, y0, -0.1],
    [mid + 0.2, y0, -0.1],
    [mid + 0.2, y0 + 0.2, -0.1],
    [mid - 0.2, y0 + 0.2, -0.1],
    [0, 0, -1],
  );
  face(
    [mid - 0.2, y0, 0.1],
    [mid + 0.2, y0, 0.1],
    [mid + 0.2, y0 + 0.2, 0.1],
    [mid - 0.2, y0 + 0.2, 0.1],
    [0, 0, 1],
  );
  face(
    [mid - 0.2, y0, -0.1],
    [mid - 0.2, y0, 0.1],
    [mid - 0.2, y0 + 0.2, 0.1],
    [mid - 0.2, y0 + 0.2, -0.1],
    [-1, 0, 0],
  );
  face(
    [mid + 0.2, y0, -0.1],
    [mid + 0.2, y0 + 0.2, -0.1],
    [mid + 0.2, y0 + 0.2, 0.1],
    [mid + 0.2, y0, 0.1],
    [1, 0, 0],
  );
  return mesh;
}

test('Cp legend is blue suction to red high pressure', () => {
  const suction = colorFromCp(-1.5);
  const high = colorFromCp(1);
  assert.ok(suction[2] > suction[0]);
  assert.ok(high[0] > high[2]);
});

test('180 yaw flips windward redness', () => {
  const rest = box();
  const count = Domain.grid[0] * Domain.grid[1] * Domain.grid[2];
  const occupancy = new Uint8Array(count);
  const macros = new Float32Array(count * 4);
  const [gx, gy, gz] = Domain.grid;
  for (let z = 0; z < gz; ++z) {
    for (let y = 0; y < gy; ++y) {
      for (let x = 0; x < gx; ++x) {
        const i = x + y * gx + z * gx * gy;
        if (x === 0) occupancy[i] = WindInlet;
        else if (x + 1 === gx) occupancy[i] = WindOutlet;
        else if (y === 0 || y + 1 === gy || z === 0 || z + 1 === gz) occupancy[i] = WindWall;
        else occupancy[i] = WindFluid;
        macros[i * 4] = 1;
        macros[i * 4 + 1] = 0.08;
      }
    }
  }
  const identity = cloneMesh(rest);
  colorMeshByPressure(identity, Domain, macros, occupancy, 0.08);
  const yawed = cloneMesh(rest);
  applyObstaclePose(yawed, { yaw: Math.PI, pitch: 0, roll: 0, translation: [0, 0, 0], scale: 1 }, Domain);
  colorMeshByPressure(yawed, Domain, macros, occupancy, 0.08);
  const split = Domain.origin[0] + 0.5 * Domain.cellSize * Domain.grid[0];
  const redness = (v) => v.color[0] - v.color[2];
  let iw = 0;
  let il = 0;
  let yw = 0;
  let yl = 0;
  let nw = 0;
  let nl = 0;
  for (let i = 0; i < rest.length; ++i) {
    if (rest[i].position[0] < split - 0.05) {
      iw += redness(identity[i]);
      yw += redness(yawed[i]);
      nw += 1;
    } else if (rest[i].position[0] > split + 0.05) {
      il += redness(identity[i]);
      yl += redness(yawed[i]);
      nl += 1;
    }
  }
  assert.ok(nw > 2 && nl > 2);
  assert.ok(iw / nw > il / nl + 0.05);
  assert.ok(yl / nl > yw / nw + 0.05);
});
