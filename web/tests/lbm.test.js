import assert from 'node:assert/strict';
import test from 'node:test';
import { Domain } from '../src/units.js';
import {
  WindFluid,
  WindInlet,
  WindOutlet,
  WindWall,
  createSolver,
  lbmFeq,
  LbmQ,
  LbmW,
  stepSolver,
} from '../src/lbm.js';

test('D3Q19 weights sum to one', () => {
  const sum = LbmW.reduce((a, b) => a + b, 0);
  assert.ok(Math.abs(sum - 1) < 1e-6);
});

test('equilibrium density is recovered', () => {
  let density = 0;
  for (let k = 0; k < LbmQ; ++k) {
    density += lbmFeq(k, 1, 0.08, 0, 0);
  }
  assert.ok(Math.abs(density - 1) < 1e-5);
});

test('empty tunnel keeps a finite inlet speed', () => {
  const count = Domain.grid[0] * Domain.grid[1] * Domain.grid[2];
  const occupancy = new Uint8Array(count);
  const [gx, gy, gz] = Domain.grid;
  for (let z = 0; z < gz; ++z) {
    for (let y = 0; y < gy; ++y) {
      for (let x = 0; x < gx; ++x) {
        const i = x + y * gx + z * gx * gy;
        if (x === 0) occupancy[i] = WindInlet;
        else if (x + 1 === gx) occupancy[i] = WindOutlet;
        else if (y === 0 || y + 1 === gy || z === 0 || z + 1 === gz) occupancy[i] = WindWall;
        else occupancy[i] = WindFluid;
      }
    }
  }
  const solver = createSolver(Domain, occupancy, 0.6, 0.08);
  stepSolver(solver, 8);
  let ux = 0;
  let n = 0;
  for (let y = 2; y + 2 < gy; ++y) {
    for (let z = 2; z + 2 < gz; ++z) {
      const i = 2 + y * gx + z * gx * gy;
      ux += solver.macros[i * 4 + 1];
      n += 1;
    }
  }
  assert.ok(n > 0);
  assert.ok(Number.isFinite(ux / n));
  assert.ok(ux / n > 0.03);
});
