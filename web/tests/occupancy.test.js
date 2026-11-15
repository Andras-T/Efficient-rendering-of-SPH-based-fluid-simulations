import assert from 'node:assert/strict';
import test from 'node:test';
import { Domain } from '../src/units.js';
import { WindSolid } from '../src/lbm.js';
import { buildWindOccupancy, frontalCells } from '../src/occupancy.js';

function boxMesh() {
  const mesh = [];
  const face = (a, b, c, d, n) => {
    for (const p of [a, b, c, a, c, d]) {
      mesh.push({ position: p.slice(), normal: n.slice(), color: [1, 1, 1] });
    }
  };
  const x0 = Domain.origin[0] + 1.4;
  const x1 = x0 + 0.48;
  const y0 = Domain.origin[1] + 0.12;
  const y1 = y0 + 0.32;
  const z0 = Domain.origin[2] + 0.4;
  const z1 = z0 + 0.32;
  face([x0, y0, z0], [x1, y0, z0], [x1, y1, z0], [x0, y1, z0], [0, 0, -1]);
  face([x0, y0, z1], [x1, y0, z1], [x1, y1, z1], [x0, y1, z1], [0, 0, 1]);
  face([x0, y0, z0], [x0, y0, z1], [x0, y1, z1], [x0, y1, z0], [-1, 0, 0]);
  face([x1, y0, z0], [x1, y0, z1], [x1, y1, z1], [x1, y1, z0], [1, 0, 0]);
  face([x0, y1, z0], [x1, y1, z0], [x1, y1, z1], [x0, y1, z1], [0, 1, 0]);
  face([x0, y0, z0], [x1, y0, z0], [x1, y0, z1], [x0, y0, z1], [0, -1, 0]);
  return mesh;
}

test('a closed box voxels into a finite frontal area', () => {
  const occupancy = buildWindOccupancy(Domain, boxMesh());
  let solids = 0;
  for (const cell of occupancy) {
    if (cell === WindSolid) {
      solids += 1;
    }
  }
  assert.ok(solids > 20);
  assert.ok(frontalCells(Domain, occupancy) > 4);
});
