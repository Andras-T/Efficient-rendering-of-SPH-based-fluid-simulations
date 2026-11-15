import assert from 'node:assert/strict';
import test from 'node:test';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, join } from 'node:path';
import { LbmOpp, LbmW } from '../src/lbm.js';

const here = dirname(fileURLToPath(import.meta.url));
const wgsl = readFileSync(join(here, '../src/shaders/lbm.wgsl'), 'utf8');

test('WGSL collide/stream match the JS D3Q19 tables', () => {
  assert.match(wgsl, /fn collide/);
  assert.match(wgsl, /fn stream/);
  assert.match(wgsl, /const LbmQ : u32 = 19u;/);
  const weightSum = LbmW.reduce((a, b) => a + b, 0);
  assert.ok(Math.abs(weightSum - 1) < 1e-6);
  assert.equal(LbmOpp[1], 2);
  assert.equal(LbmOpp[2], 1);
  assert.match(wgsl, /Opp = array<u32, 19>\(0u, 2u, 1u/);
});
