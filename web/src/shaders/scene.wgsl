struct Uniforms {
  viewProj : mat4x4<f32>,
  eye : vec4<f32>,
  light : vec4<f32>,
};

@group(0) @binding(0) var<uniform> u : Uniforms;

struct VsIn {
  @location(0) position : vec3<f32>,
  @location(1) normal : vec3<f32>,
  @location(2) color : vec3<f32>,
};

struct VsOut {
  @builtin(position) clip : vec4<f32>,
  @location(0) world : vec3<f32>,
  @location(1) normal : vec3<f32>,
  @location(2) color : vec3<f32>,
};

@vertex
fn vsCar(input : VsIn) -> VsOut {
  var out : VsOut;
  out.clip = u.viewProj * vec4<f32>(input.position, 1.0);
  out.world = input.position;
  out.normal = input.normal;
  out.color = input.color;
  return out;
}

@fragment
fn fsCar(input : VsOut) -> @location(0) vec4<f32> {
  let n = normalize(input.normal);
  let l = normalize(u.light.xyz);
  let v = normalize(u.eye.xyz - input.world);
  let ndl = max(dot(n, l), 0.0);
  let spec = pow(max(dot(n, normalize(l + v)), 0.0), 32.0);
  let lit = input.color * (0.22 + 0.78 * ndl) + vec3<f32>(0.18, 0.18, 0.2) * spec;
  return vec4<f32>(lit, 1.0);
}

struct FloorOut {
  @builtin(position) clip : vec4<f32>,
  @location(0) world : vec3<f32>,
};

@vertex
fn vsFloor(@location(0) position : vec3<f32>) -> FloorOut {
  var out : FloorOut;
  out.clip = u.viewProj * vec4<f32>(position, 1.0);
  out.world = position;
  return out;
}

@fragment
fn fsFloor(input : FloorOut) -> @location(0) vec4<f32> {
  let gx = floor(input.world.x / 0.28);
  let gz = floor(input.world.z / 0.28);
  let check = (gx + gz) % 2.0;
  let dark = vec3<f32>(0.22, 0.24, 0.28);
  let lite = vec3<f32>(0.38, 0.40, 0.44);
  let color = mix(dark, lite, check);
  let fade = smoothstep(3.2, 0.4, length(input.world.xz));
  return vec4<f32>(color * fade, 1.0);
}

@vertex
fn vsSky(@builtin(vertex_index) vertexIndex : u32) -> @builtin(position) vec4<f32> {
  var pos = array<vec2<f32>, 3>(
    vec2<f32>(-1.0, -1.0),
    vec2<f32>(3.0, -1.0),
    vec2<f32>(-1.0, 3.0)
  );
  return vec4<f32>(pos[vertexIndex], 0.999, 1.0);
}

@fragment
fn fsSky(@builtin(position) frag : vec4<f32>) -> @location(0) vec4<f32> {
  let t = clamp(frag.y / 900.0, 0.0, 1.0);
  let top = vec3<f32>(0.62, 0.78, 0.92);
  let bot = vec3<f32>(0.78, 0.86, 0.94);
  return vec4<f32>(mix(bot, top, t), 1.0);
}

struct StreakIn {
  @location(0) position : vec3<f32>,
  @location(1) color : vec3<f32>,
};

struct StreakOut {
  @builtin(position) clip : vec4<f32>,
  @location(0) color : vec3<f32>,
};

@vertex
fn vsStreak(input : StreakIn) -> StreakOut {
  var out : StreakOut;
  out.clip = u.viewProj * vec4<f32>(input.position, 1.0);
  out.color = input.color;
  return out;
}

@fragment
fn fsStreak(input : StreakOut) -> @location(0) vec4<f32> {
  return vec4<f32>(input.color, 0.82);
}
