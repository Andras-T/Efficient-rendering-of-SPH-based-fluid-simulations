import assert from 'node:assert/strict';
import test from 'node:test';
import {
  clampWindKmh,
  DefaultWindKmh,
  kmhToLattice,
  latticeToKmh,
  DefaultLatticeSpeed,
} from '../src/units.js';

test('default inlet is 180 km/h', () => {
  assert.equal(Math.round(DefaultWindKmh), 180);
  assert.ok(Math.abs(latticeToKmh(DefaultLatticeSpeed) - 180) < 0.5);
  assert.ok(Math.abs(kmhToLattice(180) - DefaultLatticeSpeed) < 1e-6);
});

test('km/h clamp stays in the HUD range', () => {
  assert.equal(clampWindKmh(10), 50);
  assert.equal(clampWindKmh(400), 280);
});
