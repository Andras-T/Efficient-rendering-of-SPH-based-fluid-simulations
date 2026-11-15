export function createCamera() {
  const portrait = typeof window !== 'undefined' && window.innerHeight > window.innerWidth * 1.05;
  return {
    target: portrait ? [-0.48, -0.18, 0] : [-0.62, 0.12, 0],
    yaw: -0.32,
    pitch: portrait ? 0.18 : 0.22,
    distance: portrait ? 2.05 : 1.55,
    fov: portrait ? 0.72 : 0.9,
    near: 0.05,
    far: 20,
  };
}

export function cameraPosition(camera) {
  const cp = Math.cos(camera.pitch);
  return [
    camera.target[0] + camera.distance * Math.sin(camera.yaw) * cp,
    camera.target[1] + camera.distance * Math.sin(camera.pitch),
    camera.target[2] + camera.distance * Math.cos(camera.yaw) * cp,
  ];
}

function normalize(v) {
  const l = Math.hypot(v[0], v[1], v[2]) || 1;
  return [v[0] / l, v[1] / l, v[2] / l];
}

function cross(a, b) {
  return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
}

function dot(a, b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

export function viewMatrix(camera) {
  const eye = cameraPosition(camera);
  const z = normalize([
    eye[0] - camera.target[0],
    eye[1] - camera.target[1],
    eye[2] - camera.target[2],
  ]);
  const x = normalize(cross([0, 1, 0], z));
  const y = cross(z, x);
  return new Float32Array([
    x[0], y[0], z[0], 0,
    x[1], y[1], z[1], 0,
    x[2], y[2], z[2], 0,
    -dot(x, eye), -dot(y, eye), -dot(z, eye), 1,
  ]);
}

export function projectionMatrix(aspect, camera) {
  const f = 1 / Math.tan(camera.fov / 2);
  const nf = 1 / (camera.near - camera.far);
  return new Float32Array([
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (camera.far + camera.near) * nf, -1,
    0, 0, 2 * camera.far * camera.near * nf, 0,
  ]);
}

export function mul4(a, b) {
  const r = new Float32Array(16);
  for (let c = 0; c < 4; ++c) {
    for (let row = 0; row < 4; ++row) {
      r[c * 4 + row] =
        a[row] * b[c * 4] +
        a[4 + row] * b[c * 4 + 1] +
        a[8 + row] * b[c * 4 + 2] +
        a[12 + row] * b[c * 4 + 3];
    }
  }
  return r;
}

export function orbit(camera, dx, dy) {
  camera.yaw -= dx * 0.0055;
  camera.pitch = Math.min(1.2, Math.max(-0.15, camera.pitch + dy * 0.0055));
}

export function pinchZoom(camera, scale) {
  camera.distance = Math.min(4.5, Math.max(0.45, camera.distance / scale));
}

export function bindCamera(canvas, camera) {
  const pointers = new Map();
  let pinch0 = 0;
  const dist = () => {
    const pts = [...pointers.values()];
    if (pts.length < 2) {
      return 0;
    }
    return Math.hypot(pts[0].x - pts[1].x, pts[0].y - pts[1].y);
  };
  canvas.addEventListener('pointerdown', (event) => {
    canvas.setPointerCapture(event.pointerId);
    pointers.set(event.pointerId, { x: event.clientX, y: event.clientY });
    if (pointers.size === 2) {
      pinch0 = dist();
    }
  });
  canvas.addEventListener('pointermove', (event) => {
    const prev = pointers.get(event.pointerId);
    if (!prev) {
      return;
    }
    if (pointers.size === 1) {
      orbit(camera, event.clientX - prev.x, event.clientY - prev.y);
    } else if (pointers.size >= 2) {
      pointers.set(event.pointerId, { x: event.clientX, y: event.clientY });
      const next = dist();
      if (pinch0 > 8 && next > 8) {
        pinchZoom(camera, next / pinch0);
        pinch0 = next;
      }
      return;
    }
    pointers.set(event.pointerId, { x: event.clientX, y: event.clientY });
  });
  const up = (event) => {
    pointers.delete(event.pointerId);
    pinch0 = dist();
  };
  canvas.addEventListener('pointerup', up);
  canvas.addEventListener('pointercancel', up);
  canvas.addEventListener(
    'wheel',
    (event) => {
      event.preventDefault();
      pinchZoom(camera, event.deltaY < 0 ? 1.08 : 0.92);
    },
    { passive: false },
  );
}
