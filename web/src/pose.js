const EPS = 1e-5;

export function identityPose() {
  return { yaw: 0, pitch: 0, roll: 0, translation: [0, 0, 0], scale: 1 };
}

export function poseIsIdentity(pose) {
  return (
    Math.abs(pose.yaw) <= EPS &&
    Math.abs(pose.pitch) <= EPS &&
    Math.abs(pose.roll) <= EPS &&
    Math.abs(pose.translation[0]) <= EPS &&
    Math.abs(pose.translation[1]) <= EPS &&
    Math.abs(pose.translation[2]) <= EPS &&
    Math.abs(pose.scale - 1) <= EPS
  );
}

function bounds(mesh) {
  const min = [Infinity, Infinity, Infinity];
  const max = [-Infinity, -Infinity, -Infinity];
  for (const v of mesh) {
    for (let i = 0; i < 3; ++i) {
      min[i] = Math.min(min[i], v.position[i]);
      max[i] = Math.max(max[i], v.position[i]);
    }
  }
  return { min, max };
}

function mulMat3(a, p) {
  return [
    a[0] * p[0] + a[1] * p[1] + a[2] * p[2],
    a[3] * p[0] + a[4] * p[1] + a[5] * p[2],
    a[6] * p[0] + a[7] * p[1] + a[8] * p[2],
  ];
}

function rotY(a) {
  const c = Math.cos(a);
  const s = Math.sin(a);
  return [c, 0, s, 0, 1, 0, -s, 0, c];
}

function rotZ(a) {
  const c = Math.cos(a);
  const s = Math.sin(a);
  return [c, -s, 0, s, c, 0, 0, 0, 1];
}

function rotX(a) {
  const c = Math.cos(a);
  const s = Math.sin(a);
  return [1, 0, 0, 0, c, -s, 0, s, c];
}

function mul3(a, b) {
  const r = new Array(9).fill(0);
  for (let i = 0; i < 3; ++i) {
    for (let j = 0; j < 3; ++j) {
      r[i * 3 + j] = a[i * 3] * b[j] + a[i * 3 + 1] * b[3 + j] + a[i * 3 + 2] * b[6 + j];
    }
  }
  return r;
}

export function cloneMesh(mesh) {
  return mesh.map((v) => ({
    position: v.position.slice(),
    normal: v.normal.slice(),
    color: v.color.slice(),
  }));
}

export function transformObstacleMesh(mesh, pose) {
  if (mesh.length === 0 || poseIsIdentity(pose)) {
    return;
  }
  const box = bounds(mesh);
  const center = [
    0.5 * (box.min[0] + box.max[0]),
    0.5 * (box.min[1] + box.max[1]),
    0.5 * (box.min[2] + box.max[2]),
  ];
  // +pitch about -Z so a nose that faces -x rises when pitch is positive.
  const axes = mul3(rotY(pose.yaw), mul3(rotZ(-pose.pitch), rotX(pose.roll)));
  const scale = Math.max(pose.scale, 0.05);
  for (const vertex of mesh) {
    const p = [
      scale * (vertex.position[0] - center[0]),
      scale * (vertex.position[1] - center[1]),
      scale * (vertex.position[2] - center[2]),
    ];
    const rp = mulMat3(axes, p);
    vertex.position[0] = center[0] + rp[0] + pose.translation[0];
    vertex.position[1] = center[1] + rp[1] + pose.translation[1];
    vertex.position[2] = center[2] + rp[2] + pose.translation[2];
    const n = mulMat3(axes, vertex.normal);
    const len = Math.hypot(n[0], n[1], n[2]);
    if (len > 1e-8) {
      vertex.normal[0] = n[0] / len;
      vertex.normal[1] = n[1] / len;
      vertex.normal[2] = n[2] / len;
    }
  }
}

export function clampMeshToLattice(mesh, domain) {
  if (mesh.length === 0) {
    return;
  }
  let box = bounds(mesh);
  const cell = domain.cellSize;
  const interiorMin = [
    domain.origin[0] + 2 * cell,
    domain.origin[1] + 1.25 * cell,
    domain.origin[2] + 1.5 * cell,
  ];
  const interiorMax = [
    domain.origin[0] + domain.grid[0] * cell - 2 * cell,
    domain.origin[1] + domain.grid[1] * cell - 1.5 * cell,
    domain.origin[2] + domain.grid[2] * cell - 1.5 * cell,
  ];
  const room = [
    interiorMax[0] - interiorMin[0],
    interiorMax[1] - interiorMin[1],
    interiorMax[2] - interiorMin[2],
  ];
  const extent = [
    box.max[0] - box.min[0],
    box.max[1] - box.min[1],
    box.max[2] - box.min[2],
  ];
  let shrink = 1;
  for (let axis = 0; axis < 3; ++axis) {
    if (room[axis] > 1e-5 && extent[axis] > room[axis]) {
      shrink = Math.min(shrink, (0.98 * room[axis]) / extent[axis]);
    }
  }
  if (shrink < 1) {
    const center = [
      0.5 * (box.min[0] + box.max[0]),
      0.5 * (box.min[1] + box.max[1]),
      0.5 * (box.min[2] + box.max[2]),
    ];
    for (const vertex of mesh) {
      vertex.position[0] = center[0] + shrink * (vertex.position[0] - center[0]);
      vertex.position[1] = center[1] + shrink * (vertex.position[1] - center[1]);
      vertex.position[2] = center[2] + shrink * (vertex.position[2] - center[2]);
    }
    box = bounds(mesh);
  }
  const shift = [0, 0, 0];
  for (let axis = 0; axis < 3; ++axis) {
    if (box.min[axis] < interiorMin[axis]) {
      shift[axis] = interiorMin[axis] - box.min[axis];
    } else if (box.max[axis] > interiorMax[axis]) {
      shift[axis] = interiorMax[axis] - box.max[axis];
    }
  }
  if (shift[0] * shift[0] + shift[1] * shift[1] + shift[2] * shift[2] > 1e-12) {
    for (const vertex of mesh) {
      vertex.position[0] += shift[0];
      vertex.position[1] += shift[1];
      vertex.position[2] += shift[2];
    }
  }
}

export function applyObstaclePose(mesh, pose, domain) {
  transformObstacleMesh(mesh, pose);
  if (!poseIsIdentity(pose)) {
    clampMeshToLattice(mesh, domain);
  }
}

export function meshBounds(mesh) {
  return bounds(mesh);
}
