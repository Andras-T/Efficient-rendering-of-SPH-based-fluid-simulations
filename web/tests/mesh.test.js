import assert from 'node:assert/strict';
import test from 'node:test';
import { readFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';
import { Domain } from '../src/units.js';
import { fitMeshToTunnel, parsePackedMesh } from '../src/mesh.js';

const packed = readFileSync(join(dirname(fileURLToPath(import.meta.url)), '../public/c42.bin'));

test('packed C42 is a closed triangle body', () => {
  const mesh = parsePackedMesh(packed.buffer.slice(packed.byteOffset, packed.byteOffset + packed.byteLength));
  assert.equal(mesh.length % 3, 0);
  assert.ok(mesh.length / 3 > 80_000);
  fitMeshToTunnel(mesh, Domain);
  let minX = Infinity;
  let maxX = -Infinity;
  let minY = Infinity;
  let maxY = -Infinity;
  for (const v of mesh) {
    minX = Math.min(minX, v.position[0]);
    maxX = Math.max(maxX, v.position[0]);
    minY = Math.min(minY, v.position[1]);
    maxY = Math.max(maxY, v.position[1]);
  }
  const length = maxX - minX;
  const height = maxY - minY;
  assert.ok(length > height);
  assert.ok(length > 1.0);
  assert.ok(height > 0.3);
});
