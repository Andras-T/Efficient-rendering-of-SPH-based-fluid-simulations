import assert from 'node:assert/strict';
import test from 'node:test';
import { Domain } from '../src/units.js';
import { applyObstaclePose, cloneMesh, identityPose } from '../src/pose.js';

function box() {
  const mesh = [];
  const push = (p, n) => mesh.push({ position: p, normal: n, color: [1, 1, 1] });
  const q = (a, b, c, d, n) => {
    push(a, n.slice());
    push(b, n.slice());
    push(c, n.slice());
    push(a, n.slice());
    push(c, n.slice());
    push(d, n.slice());
  };
  q([-0.4, 0.2, -0.12], [0.4, 0.2, -0.12], [0.4, 0.4, -0.12], [-0.4, 0.4, -0.12], [0, 0, -1]);
  q([-0.4, 0.2, 0.12], [0.4, 0.2, 0.12], [0.4, 0.4, 0.12], [-0.4, 0.4, 0.12], [0, 0, 1]);
  q([-0.4, 0.2, -0.12], [-0.4, 0.2, 0.12], [-0.4, 0.4, 0.12], [-0.4, 0.4, -0.12], [-1, 0, 0]);
  q([0.4, 0.2, -0.12], [0.4, 0.2, 0.12], [0.4, 0.4, 0.12], [0.4, 0.4, -0.12], [1, 0, 0]);
  return mesh;
}

function extent(mesh, axis) {
  let min = Infinity;
  let max = -Infinity;
  for (const v of mesh) {
    min = Math.min(min, v.position[axis]);
    max = Math.max(max, v.position[axis]);
  }
  return max - min;
}

test('identity pose leaves vertices alone', () => {
  const mesh = box();
  const x = mesh[0].position[0];
  applyObstaclePose(mesh, identityPose(), Domain);
  assert.ok(Math.abs(mesh[0].position[0] - x) < 1e-6);
});

test('90 degree yaw swaps length and width', () => {
  const rest = box();
  const yawed = cloneMesh(rest);
  applyObstaclePose(yawed, { yaw: Math.PI / 2, pitch: 0, roll: 0, translation: [0, 0, 0], scale: 1 }, Domain);
  assert.ok(Math.abs(extent(yawed, 0) - extent(rest, 2)) < 0.05);
  assert.ok(Math.abs(extent(yawed, 2) - extent(rest, 0)) < 0.05);
});
