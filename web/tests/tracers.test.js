import assert from 'node:assert/strict';
import test from 'node:test';
import { Domain } from '../src/units.js';
import { WindFluid, WindInlet, WindOutlet, WindSolid, WindWall, createSolver } from '../src/lbm.js';
import { createTracers, packStreaks, sampleVelocity, stepTracers, TracerCount } from '../src/tracers.js';

function emptyOccupancy() {
  const [gx, gy, gz] = Domain.grid;
  const occupancy = new Uint8Array(gx * gy * gz);
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
  return occupancy;
}

test('inlet rake seeds a finite tracer count', () => {
  const solver = createSolver(Domain, emptyOccupancy(), 0.6, 0.08);
  const tracers = createTracers(solver, Domain);
  assert.equal(tracers.length, TracerCount);
  assert.ok(TracerCount > 400);
  const packed = packStreaks(tracers, 0.08);
  assert.equal(packed.length, TracerCount * 36);
});

test('tracers advect with the inlet wind', () => {
  const solver = createSolver(Domain, emptyOccupancy(), 0.6, 0.08);
  const tracers = createTracers(solver, Domain);
  const x0 = tracers.reduce((s, t) => s + t.position[0], 0) / tracers.length;
  stepTracers(tracers, solver, 6, Domain);
  const x1 = tracers.reduce((s, t) => s + t.position[0], 0) / tracers.length;
  assert.ok(x1 > x0 + 0.01);
  const u = sampleVelocity(solver, [
    Domain.origin[0] + Domain.cellSize * 4,
    Domain.origin[1] + Domain.cellSize * 8,
    Domain.origin[2] + Domain.cellSize * 8,
  ]);
  assert.ok(u[0] > 0.03);
});

test('a solid cell does not swallow the rake', () => {
  const occupancy = emptyOccupancy();
  const [gx, gy] = Domain.grid;
  occupancy[8 + 8 * gx + 8 * gx * gy] = WindSolid;
  const solver = createSolver(Domain, occupancy, 0.6, 0.08);
  const tracers = createTracers(solver, Domain);
  stepTracers(tracers, solver, 4, Domain);
  assert.equal(tracers.length, TracerCount);
  assert.ok(tracers.every((t) => Number.isFinite(t.position[0])));
});
