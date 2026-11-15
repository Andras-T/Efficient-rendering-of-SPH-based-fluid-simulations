const WindFluid : u32 = 0u;
const WindSolid : u32 = 1u;
const WindInlet : u32 = 2u;
const WindOutlet : u32 = 3u;
const WindWall : u32 = 4u;
const LbmQ : u32 = 19u;

const Ex = array<i32, 19>(0, 1, -1, 0, 0, 0, 0, 1, -1, 1, -1, 1, -1, 1, -1, 0, 0, 0, 0);
const Ey = array<i32, 19>(0, 0, 0, 1, -1, 0, 0, 1, 1, -1, -1, 0, 0, 0, 0, 1, -1, 1, -1);
const Ez = array<i32, 19>(0, 0, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1, 1, -1, -1, 1, 1, -1, -1);
const Opp = array<u32, 19>(0u, 2u, 1u, 4u, 3u, 6u, 5u, 10u, 9u, 8u, 7u, 14u, 13u, 12u, 11u, 18u, 17u, 16u, 15u);
const W = array<f32, 19>(
  0.33333334,
  0.05555556, 0.05555556, 0.05555556, 0.05555556, 0.05555556, 0.05555556,
  0.02777778, 0.02777778, 0.02777778, 0.02777778, 0.02777778, 0.02777778,
  0.02777778, 0.02777778, 0.02777778, 0.02777778, 0.02777778, 0.02777778
);

struct Params {
  grid : vec4<u32>,
  tau : f32,
  inletUx : f32,
  cellCount : u32,
  _pad : u32,
};

@group(0) @binding(0) var<uniform> params : Params;
@group(0) @binding(1) var<storage, read> occupancy : array<u32>;
@group(0) @binding(2) var<storage, read> src : array<f32>;
@group(0) @binding(3) var<storage, read_write> dst : array<f32>;
@group(0) @binding(4) var<storage, read_write> macros : array<vec4<f32>>;

fn feq(k : u32, density : f32, ux : f32, uy : f32, uz : f32) -> f32 {
  let eu = f32(Ex[k]) * ux + f32(Ey[k]) * uy + f32(Ez[k]) * uz;
  let uu = ux * ux + uy * uy + uz * uz;
  return W[k] * density * (1.0 + 3.0 * eu + 4.5 * eu * eu - 1.5 * uu);
}

fn idx3(x : i32, y : i32, z : i32) -> u32 {
  return u32(x) + u32(y) * params.grid.x + u32(z) * params.grid.x * params.grid.y;
}

@compute @workgroup_size(64)
fn collide(@builtin(global_invocation_id) gid : vec3<u32>) {
  let i = gid.x;
  if (i >= params.cellCount) { return; }
  let occ = occupancy[i];
  let base = i * LbmQ;
  if (occ == WindSolid || occ == WindWall) {
    for (var k = 0u; k < LbmQ; k++) { dst[base + k] = src[base + k]; }
    macros[i] = vec4<f32>(1.0, 0.0, 0.0, 0.0);
    return;
  }
  var density = 0.0;
  var mx = 0.0;
  var my = 0.0;
  var mz = 0.0;
  for (var k = 0u; k < LbmQ; k++) {
    let fk = src[base + k];
    density += fk;
    mx += fk * f32(Ex[k]);
    my += fk * f32(Ey[k]);
    mz += fk * f32(Ez[k]);
  }
  density = max(density, 1.0e-5);
  var ux = mx / density;
  var uy = my / density;
  var uz = mz / density;
  if (occ == WindInlet) {
    density = 1.0;
    ux = params.inletUx;
    uy = 0.0;
    uz = 0.0;
  }
  let invTau = 1.0 / params.tau;
  for (var k = 0u; k < LbmQ; k++) {
    let eq = feq(k, density, ux, uy, uz);
    dst[base + k] = src[base + k] + (eq - src[base + k]) * invTau;
  }
  macros[i] = vec4<f32>(density, ux, uy, uz);
}

@compute @workgroup_size(64)
fn stream(@builtin(global_invocation_id) gid : vec3<u32>) {
  let i = gid.x;
  if (i >= params.cellCount) { return; }
  let occ = occupancy[i];
  let base = i * LbmQ;
  let gx = i32(params.grid.x);
  let gy = i32(params.grid.y);
  let gz = i32(params.grid.z);
  let x = i32(i % params.grid.x);
  let y = i32((i / params.grid.x) % params.grid.y);
  let z = i32(i / (params.grid.x * params.grid.y));
  if (occ == WindSolid || occ == WindWall) {
    for (var k = 0u; k < LbmQ; k++) { dst[base + k] = src[base + k]; }
    return;
  }
  if (occ == WindInlet) {
    for (var k = 0u; k < LbmQ; k++) {
      dst[base + k] = feq(k, 1.0, params.inletUx, 0.0, 0.0);
    }
    macros[i] = vec4<f32>(1.0, params.inletUx, 0.0, 0.0);
    return;
  }
  if (occ == WindOutlet && x > 0) {
    let left = idx3(x - 1, y, z) * LbmQ;
    for (var k = 0u; k < LbmQ; k++) { dst[base + k] = src[left + k]; }
    return;
  }
  for (var k = 0u; k < LbmQ; k++) {
    let sx = x - Ex[k];
    let sy = y - Ey[k];
    let sz = z - Ez[k];
    if (sx < 0 || sy < 0 || sz < 0 || sx >= gx || sy >= gy || sz >= gz) {
      dst[base + k] = src[base + Opp[k]];
      continue;
    }
    let srcIndex = idx3(sx, sy, sz);
    let srcOcc = occupancy[srcIndex];
    if (srcOcc == WindSolid || srcOcc == WindWall) {
      dst[base + k] = src[base + Opp[k]];
    } else {
      dst[base + k] = src[srcIndex * LbmQ + k];
    }
  }
}
