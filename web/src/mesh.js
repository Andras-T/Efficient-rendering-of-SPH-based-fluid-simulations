export function parseStl(buffer) {
  const bytes = buffer instanceof ArrayBuffer ? new Uint8Array(buffer) : buffer;
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const count = view.getUint32(80, true);
  const mesh = [];
  let offset = 84;
  for (let i = 0; i < count; ++i) {
    const nx = view.getFloat32(offset, true);
    const ny = view.getFloat32(offset + 4, true);
    const nz = view.getFloat32(offset + 8, true);
    const verts = [];
    for (let v = 0; v < 3; ++v) {
      verts.push([
        view.getFloat32(offset + 12 + v * 12, true),
        view.getFloat32(offset + 16 + v * 12, true),
        view.getFloat32(offset + 20 + v * 12, true),
      ]);
    }
    offset += 50;
    const ab = [verts[1][0] - verts[0][0], verts[1][1] - verts[0][1], verts[1][2] - verts[0][2]];
    const ac = [verts[2][0] - verts[0][0], verts[2][1] - verts[0][1], verts[2][2] - verts[0][2]];
    let normal = [
      ab[1] * ac[2] - ab[2] * ac[1],
      ab[2] * ac[0] - ab[0] * ac[2],
      ab[0] * ac[1] - ab[1] * ac[0],
    ];
    let len = Math.hypot(normal[0], normal[1], normal[2]);
    if (len < 1e-12) {
      continue;
    }
    const nlen = Math.hypot(nx, ny, nz);
    if (nlen > 1e-8) {
      normal = [nx / nlen, ny / nlen, nz / nlen];
    } else {
      normal = [normal[0] / len, normal[1] / len, normal[2] / len];
    }
    for (const position of verts) {
      mesh.push({ position, normal: normal.slice(), color: [0.82, 0.84, 0.88] });
    }
  }
  if (mesh.length < 3) {
    throw new Error('mesh has no triangles');
  }
  return mesh;
}

export function fitMeshToTunnel(mesh, domain) {
  if (mesh.length === 0) {
    return;
  }
  const min = [Infinity, Infinity, Infinity];
  const max = [-Infinity, -Infinity, -Infinity];
  for (const vertex of mesh) {
    for (let i = 0; i < 3; ++i) {
      min[i] = Math.min(min[i], vertex.position[i]);
      max[i] = Math.max(max[i], vertex.position[i]);
    }
  }
  const extent = [
    Math.max(max[0] - min[0], 1e-4),
    Math.max(max[1] - min[1], 1e-4),
    Math.max(max[2] - min[2], 1e-4),
  ];
  const domainSize = [
    domain.cellSize * domain.grid[0],
    domain.cellSize * domain.grid[1],
    domain.cellSize * domain.grid[2],
  ];
  const scale = Math.min(
    (domainSize[1] * 0.52) / extent[1],
    (domainSize[0] * 0.44) / extent[0],
    (domainSize[2] * 0.58) / extent[2],
    8,
  );
  const center = [0.5 * (min[0] + max[0]), 0.5 * (min[1] + max[1]), 0.5 * (min[2] + max[2])];
  const tunnelMid = [
    domain.origin[0] + 0.5 * domainSize[0],
    domain.origin[1] + 0.5 * domainSize[1],
    domain.origin[2] + 0.5 * domainSize[2],
  ];
  const floorY = domain.origin[1] + domain.cellSize * 1.25;
  const scaledMinY = (min[1] - center[1]) * scale;
  const translation = [
    tunnelMid[0] - 0.18 * domainSize[0],
    floorY - scaledMinY,
    tunnelMid[2],
  ];
  for (const vertex of mesh) {
    vertex.position[0] = (vertex.position[0] - center[0]) * scale + translation[0];
    vertex.position[1] = (vertex.position[1] - center[1]) * scale + translation[1];
    vertex.position[2] = (vertex.position[2] - center[2]) * scale + translation[2];
  }
}

export function packMesh(mesh) {
  const data = new Float32Array(mesh.length * 9);
  for (let i = 0; i < mesh.length; ++i) {
    const o = i * 9;
    data[o] = mesh[i].position[0];
    data[o + 1] = mesh[i].position[1];
    data[o + 2] = mesh[i].position[2];
    data[o + 3] = mesh[i].normal[0];
    data[o + 4] = mesh[i].normal[1];
    data[o + 5] = mesh[i].normal[2];
    data[o + 6] = mesh[i].color[0];
    data[o + 7] = mesh[i].color[1];
    data[o + 8] = mesh[i].color[2];
  }
  return data;
}
