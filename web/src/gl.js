import { cameraPosition, mul4, projectionMatrix, viewMatrix } from './camera.js';
import { packMesh } from './mesh.js';
import { Domain } from './units.js';

const GlOpts = {
  alpha: false,
  antialias: true,
  depth: true,
  stencil: false,
  premultipliedAlpha: true,
  preserveDrawingBuffer: true,
  powerPreference: 'high-performance',
};

export function requestGl(canvas) {
  const gl2 = canvas.getContext('webgl2', GlOpts);
  if (gl2) {
    return { ok: true, gl: gl2, version: 2 };
  }
  const gl1 =
    canvas.getContext('webgl', GlOpts) || canvas.getContext('experimental-webgl', GlOpts);
  if (gl1) {
    return { ok: true, gl: gl1, version: 1 };
  }
  return { ok: false, reason: 'This browser has no WebGL. Try Safari or Chrome on a recent phone.' };
}

function compile(gl, type, source) {
  const shader = gl.createShader(type);
  gl.shaderSource(shader, source);
  gl.compileShader(shader);
  if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
    const log = gl.getShaderInfoLog(shader);
    gl.deleteShader(shader);
    throw new Error(log || 'shader compile failed');
  }
  return shader;
}

function program(gl, vs, fs) {
  const prog = gl.createProgram();
  gl.attachShader(prog, compile(gl, gl.VERTEX_SHADER, vs));
  gl.attachShader(prog, compile(gl, gl.FRAGMENT_SHADER, fs));
  gl.linkProgram(prog);
  if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) {
    throw new Error(gl.getProgramInfoLog(prog) || 'shader link failed');
  }
  return prog;
}

function sources(version) {
  if (version >= 2) {
    return {
      carVs: `#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;
layout(location=2) in vec3 aCol;
uniform mat4 uViewProj;
out vec3 vWorld;
out vec3 vNrm;
out vec3 vCol;
void main() {
  vWorld = aPos;
  vNrm = aNrm;
  vCol = aCol;
  gl_Position = uViewProj * vec4(aPos, 1.0);
}`,
      carFs: `#version 300 es
precision highp float;
in vec3 vWorld;
in vec3 vNrm;
in vec3 vCol;
uniform vec3 uEye;
uniform vec3 uLight;
out vec4 frag;
void main() {
  vec3 n = normalize(vNrm);
  vec3 l = normalize(uLight);
  vec3 v = normalize(uEye - vWorld);
  float ndl = max(dot(n, l), 0.0);
  float spec = pow(max(dot(n, normalize(l + v)), 0.0), 32.0);
  frag = vec4(vCol * (0.22 + 0.78 * ndl) + vec3(0.18, 0.18, 0.2) * spec, 1.0);
}`,
      floorVs: `#version 300 es
layout(location=0) in vec3 aPos;
uniform mat4 uViewProj;
out vec3 vWorld;
void main() {
  vWorld = aPos;
  gl_Position = uViewProj * vec4(aPos, 1.0);
}`,
      floorFs: `#version 300 es
precision highp float;
in vec3 vWorld;
out vec4 frag;
void main() {
  float gx = floor(vWorld.x / 0.28);
  float gz = floor(vWorld.z / 0.28);
  float check = mod(gx + gz, 2.0);
  vec3 color = mix(vec3(0.22, 0.24, 0.28), vec3(0.38, 0.40, 0.44), check);
  float fade = smoothstep(3.2, 0.4, length(vWorld.xz));
  frag = vec4(color * fade, 1.0);
}`,
      skyVs: `#version 300 es
const vec2 pos[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
void main() {
  gl_Position = vec4(pos[gl_VertexID], 0.999, 1.0);
}`,
      skyFs: `#version 300 es
precision highp float;
out vec4 frag;
void main() {
  float t = clamp(gl_FragCoord.y / 900.0, 0.0, 1.0);
  frag = vec4(mix(vec3(0.78, 0.86, 0.94), vec3(0.62, 0.78, 0.92), t), 1.0);
}`,
      streakVs: `#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aCol;
uniform mat4 uViewProj;
out vec3 vCol;
void main() {
  vCol = aCol;
  gl_Position = uViewProj * vec4(aPos, 1.0);
}`,
      streakFs: `#version 300 es
precision highp float;
in vec3 vCol;
out vec4 frag;
void main() {
  frag = vec4(vCol, 0.82);
}`,
    };
  }
  return {
    carVs: `
attribute vec3 aPos;
attribute vec3 aNrm;
attribute vec3 aCol;
uniform mat4 uViewProj;
varying vec3 vWorld;
varying vec3 vNrm;
varying vec3 vCol;
void main() {
  vWorld = aPos;
  vNrm = aNrm;
  vCol = aCol;
  gl_Position = uViewProj * vec4(aPos, 1.0);
}`,
    carFs: `
precision mediump float;
varying vec3 vWorld;
varying vec3 vNrm;
varying vec3 vCol;
uniform vec3 uEye;
uniform vec3 uLight;
void main() {
  vec3 n = normalize(vNrm);
  vec3 l = normalize(uLight);
  vec3 v = normalize(uEye - vWorld);
  float ndl = max(dot(n, l), 0.0);
  float spec = pow(max(dot(n, normalize(l + v)), 0.0), 32.0);
  gl_FragColor = vec4(vCol * (0.22 + 0.78 * ndl) + vec3(0.18, 0.18, 0.2) * spec, 1.0);
}`,
    floorVs: `
attribute vec3 aPos;
uniform mat4 uViewProj;
varying vec3 vWorld;
void main() {
  vWorld = aPos;
  gl_Position = uViewProj * vec4(aPos, 1.0);
}`,
    floorFs: `
precision mediump float;
varying vec3 vWorld;
void main() {
  float gx = floor(vWorld.x / 0.28);
  float gz = floor(vWorld.z / 0.28);
  float check = mod(gx + gz, 2.0);
  vec3 color = mix(vec3(0.22, 0.24, 0.28), vec3(0.38, 0.40, 0.44), check);
  float fade = smoothstep(3.2, 0.4, length(vWorld.xz));
  gl_FragColor = vec4(color * fade, 1.0);
}`,
    skyVs: `
attribute vec2 aPos;
void main() {
  gl_Position = vec4(aPos, 0.999, 1.0);
}`,
    skyFs: `
precision mediump float;
void main() {
  float t = clamp(gl_FragCoord.y / 900.0, 0.0, 1.0);
  gl_FragColor = vec4(mix(vec3(0.78, 0.86, 0.94), vec3(0.62, 0.78, 0.92), t), 1.0);
}`,
    streakVs: `
attribute vec3 aPos;
attribute vec3 aCol;
uniform mat4 uViewProj;
varying vec3 vCol;
void main() {
  vCol = aCol;
  gl_Position = uViewProj * vec4(aPos, 1.0);
}`,
    streakFs: `
precision mediump float;
varying vec3 vCol;
void main() {
  gl_FragColor = vec4(vCol, 0.82);
}`,
  };
}

function bindAttrib(gl, loc, size, stride, offset) {
  gl.enableVertexAttribArray(loc);
  gl.vertexAttribPointer(loc, size, gl.FLOAT, false, stride, offset);
}

export function createGlRenderer(gpu, domain = Domain) {
  const { gl, version } = gpu;
  const src = sources(version);
  const car = program(gl, src.carVs, src.carFs);
  const floor = program(gl, src.floorVs, src.floorFs);
  const sky = program(gl, src.skyVs, src.skyFs);
  const streak = program(gl, src.streakVs, src.streakFs);
  const carView = gl.getUniformLocation(car, 'uViewProj');
  const carEye = gl.getUniformLocation(car, 'uEye');
  const carLight = gl.getUniformLocation(car, 'uLight');
  const floorView = gl.getUniformLocation(floor, 'uViewProj');
  const streakView = gl.getUniformLocation(streak, 'uViewProj');

  const o = domain.origin;
  const m = [
    o[0] + domain.cellSize * domain.grid[0],
    o[1],
    o[2] + domain.cellSize * domain.grid[2],
  ];
  const y = o[1] + 0.002;
  const floorData = new Float32Array([
    o[0], y, o[2], m[0], y, o[2], m[0], y, m[2],
    o[0], y, o[2], m[0], y, m[2], o[0], y, m[2],
  ]);
  const floorBuf = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, floorBuf);
  gl.bufferData(gl.ARRAY_BUFFER, floorData, gl.STATIC_DRAW);

  const skyData = new Float32Array([-1, -1, 3, -1, -1, 3]);
  const skyBuf = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, skyBuf);
  gl.bufferData(gl.ARRAY_BUFFER, skyData, gl.STATIC_DRAW);

  const meshBuf = gl.createBuffer();
  let meshCount = 0;
  const tracerBuf = gl.createBuffer();
  let tracerCount = 0;

  const loc = (prog, name, fallback) => {
    const found = gl.getAttribLocation(prog, name);
    return found >= 0 ? found : fallback;
  };
  const carPos = loc(car, 'aPos', 0);
  const carNrm = loc(car, 'aNrm', 1);
  const carCol = loc(car, 'aCol', 2);
  const floorPos = loc(floor, 'aPos', 0);
  const skyPos = loc(sky, 'aPos', 0);
  const streakPos = loc(streak, 'aPos', 0);
  const streakCol = loc(streak, 'aCol', 1);

  gl.enable(gl.DEPTH_TEST);
  gl.disable(gl.CULL_FACE);

  const setView = (camera, width, height) => {
    const view = viewMatrix(camera);
    const proj = projectionMatrix(width / Math.max(height, 1), camera);
    const viewProj = mul4(proj, view);
    const eye = cameraPosition(camera);
    return { viewProj, eye };
  };

  const onlyAttribs = (...locs) => {
    const used = new Set(locs.filter((loc) => loc >= 0));
    for (let i = 0; i < 8; ++i) {
      if (used.has(i)) {
        gl.enableVertexAttribArray(i);
      } else {
        gl.disableVertexAttribArray(i);
      }
    }
  };

  const drawScene = (camera, width, height) => {
    gl.viewport(0, 0, width, height);
    gl.clearColor(0.72, 0.82, 0.9, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
    const { viewProj, eye } = setView(camera, width, height);

    gl.depthFunc(gl.ALWAYS);
    gl.depthMask(false);
    gl.useProgram(sky);
    if (version < 2) {
      gl.bindBuffer(gl.ARRAY_BUFFER, skyBuf);
      bindAttrib(gl, skyPos, 2, 8, 0);
      onlyAttribs(skyPos);
    } else {
      onlyAttribs();
    }
    gl.drawArrays(gl.TRIANGLES, 0, 3);

    gl.depthFunc(gl.LEQUAL);
    gl.depthMask(true);
    gl.useProgram(floor);
    gl.uniformMatrix4fv(floorView, false, viewProj);
    gl.bindBuffer(gl.ARRAY_BUFFER, floorBuf);
    bindAttrib(gl, floorPos, 3, 12, 0);
    onlyAttribs(floorPos);
    gl.drawArrays(gl.TRIANGLES, 0, 6);

    if (meshCount > 0) {
      gl.useProgram(car);
      gl.uniformMatrix4fv(carView, false, viewProj);
      gl.uniform3f(carEye, eye[0], eye[1], eye[2]);
      gl.uniform3f(carLight, -0.45, 0.82, 0.35);
      gl.bindBuffer(gl.ARRAY_BUFFER, meshBuf);
      bindAttrib(gl, carPos, 3, 36, 0);
      bindAttrib(gl, carNrm, 3, 36, 12);
      bindAttrib(gl, carCol, 3, 36, 24);
      onlyAttribs(carPos, carNrm, carCol);
      gl.drawArrays(gl.TRIANGLES, 0, meshCount);
    }
    if (tracerCount > 0) {
      gl.enable(gl.BLEND);
      gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
      gl.depthMask(false);
      gl.useProgram(streak);
      gl.uniformMatrix4fv(streakView, false, viewProj);
      gl.bindBuffer(gl.ARRAY_BUFFER, tracerBuf);
      bindAttrib(gl, streakPos, 3, 24, 0);
      bindAttrib(gl, streakCol, 3, 24, 12);
      onlyAttribs(streakPos, streakCol);
      gl.drawArrays(gl.TRIANGLES, 0, tracerCount);
      gl.depthMask(true);
      gl.disable(gl.BLEND);
    }
  };

  return {
    api: version >= 2 ? 'WebGL2' : 'WebGL',
    uploadMesh(mesh) {
      const packed = packMesh(mesh);
      meshCount = mesh.length;
      gl.bindBuffer(gl.ARRAY_BUFFER, meshBuf);
      gl.bufferData(gl.ARRAY_BUFFER, packed, gl.DYNAMIC_DRAW);
    },
    uploadTracers(packed) {
      tracerCount = packed.length / 6;
      gl.bindBuffer(gl.ARRAY_BUFFER, tracerBuf);
      gl.bufferData(gl.ARRAY_BUFFER, packed, gl.DYNAMIC_DRAW);
    },
    draw(camera, canvas) {
      const w = canvas.width;
      const h = canvas.height;
      if (w < 2 || h < 2) {
        return;
      }
      drawScene(camera, w, h);
    },
    capture(camera, width, height) {
      const w = Math.max(2, Math.floor(width));
      const h = Math.max(2, Math.floor(height));
      drawScene(camera, w, h);
      const packed = new Uint8Array(w * h * 4);
      gl.readPixels(0, 0, w, h, gl.RGBA, gl.UNSIGNED_BYTE, packed);
      const rgba = new Uint8ClampedArray(w * h * 4);
      for (let y = 0; y < h; ++y) {
        const src = (h - 1 - y) * w * 4;
        rgba.set(packed.subarray(src, src + w * 4), y * w * 4);
      }
      return { width: w, height: h, rgba };
    },
  };
}
