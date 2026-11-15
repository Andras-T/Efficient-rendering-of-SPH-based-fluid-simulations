import sceneWgsl from './shaders/scene.wgsl?raw';
import lbmWgsl from './shaders/lbm.wgsl?raw';
import { cameraPosition, mul4, projectionMatrix, viewMatrix } from './camera.js';
import { packMesh } from './mesh.js';
import { cellCount, Domain } from './units.js';
import { LbmQ } from './lbm.js';

export function preferWebgl() {
  if (typeof window === 'undefined') {
    return false;
  }
  const query = new URLSearchParams(window.location.search);
  return query.has('gl') || query.get('webgpu') === '0';
}

export async function requestGpu(canvas) {
  if (preferWebgl()) {
    return { ok: false, skipped: true, reason: 'WebGPU skipped' };
  }
  if (!navigator.gpu) {
    return { ok: false, reason: 'This browser has no WebGPU.' };
  }
  const adapter = await navigator.gpu.requestAdapter();
  if (!adapter) {
    return { ok: false, reason: 'WebGPU adapter request failed. The GPU may be blocked or unsupported.' };
  }
  const device = await adapter.requestDevice();
  device.addEventListener('uncapturederror', (event) => {
    console.error('WebGPU', event.error?.message ?? event.error);
  });
  const context = canvas.getContext('webgpu');
  if (!context) {
    return { ok: false, reason: 'The canvas could not create a WebGPU context.' };
  }
  const format = navigator.gpu.getPreferredCanvasFormat();
  context.configure({ device, format, alphaMode: 'opaque' });
  return { ok: true, device, context, format, adapter };
}

function createBuffer(device, data, usage) {
  const buffer = device.createBuffer({
    size: data.byteLength,
    usage,
    mappedAtCreation: true,
  });
  new Uint8Array(buffer.getMappedRange()).set(new Uint8Array(data.buffer, data.byteOffset, data.byteLength));
  buffer.unmap();
  return buffer;
}

export function createRenderer(gpu, domain = Domain) {
  const { device, context, format } = gpu;
  const shader = device.createShaderModule({ code: sceneWgsl });
  const uniformBuffer = device.createBuffer({
    size: 96,
    usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST,
  });
  const bgl = device.createBindGroupLayout({
    entries: [{ binding: 0, visibility: GPUShaderStage.VERTEX | GPUShaderStage.FRAGMENT, buffer: {} }],
  });
  const layout = device.createPipelineLayout({ bindGroupLayouts: [bgl] });
  const bindGroup = device.createBindGroup({ layout: bgl, entries: [{ binding: 0, resource: { buffer: uniformBuffer } }] });
  const carPipeline = device.createRenderPipeline({
    layout,
    vertex: {
      module: shader,
      entryPoint: 'vsCar',
      buffers: [{
        arrayStride: 36,
        attributes: [
          { shaderLocation: 0, offset: 0, format: 'float32x3' },
          { shaderLocation: 1, offset: 12, format: 'float32x3' },
          { shaderLocation: 2, offset: 24, format: 'float32x3' },
        ],
      }],
    },
    fragment: { module: shader, entryPoint: 'fsCar', targets: [{ format }] },
    primitive: { topology: 'triangle-list', cullMode: 'none' },
    depthStencil: { format: 'depth24plus', depthWriteEnabled: true, depthCompare: 'less' },
  });
  const floorPipeline = device.createRenderPipeline({
    layout,
    vertex: {
      module: shader,
      entryPoint: 'vsFloor',
      buffers: [{
        arrayStride: 12,
        attributes: [{ shaderLocation: 0, offset: 0, format: 'float32x3' }],
      }],
    },
    fragment: { module: shader, entryPoint: 'fsFloor', targets: [{ format }] },
    primitive: { topology: 'triangle-list' },
    depthStencil: { format: 'depth24plus', depthWriteEnabled: true, depthCompare: 'less' },
  });
  const skyPipeline = device.createRenderPipeline({
    layout,
    vertex: { module: shader, entryPoint: 'vsSky' },
    fragment: { module: shader, entryPoint: 'fsSky', targets: [{ format }] },
    primitive: { topology: 'triangle-list' },
    depthStencil: { format: 'depth24plus', depthWriteEnabled: false, depthCompare: 'always' },
  });
  const streakBlend = {
    color: { srcFactor: 'src-alpha', dstFactor: 'one-minus-src-alpha', operation: 'add' },
    alpha: { srcFactor: 'one', dstFactor: 'one-minus-src-alpha', operation: 'add' },
  };
  const streakPipeline = device.createRenderPipeline({
    layout,
    vertex: {
      module: shader,
      entryPoint: 'vsStreak',
      buffers: [{
        arrayStride: 24,
        attributes: [
          { shaderLocation: 0, offset: 0, format: 'float32x3' },
          { shaderLocation: 1, offset: 12, format: 'float32x3' },
        ],
      }],
    },
    fragment: { module: shader, entryPoint: 'fsStreak', targets: [{ format, blend: streakBlend }] },
    primitive: { topology: 'triangle-list', cullMode: 'none' },
    depthStencil: { format: 'depth24plus', depthWriteEnabled: false, depthCompare: 'less' },
  });

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
  const floorBuffer = createBuffer(device, floorData, GPUBufferUsage.VERTEX);

  let meshBuffer = null;
  let meshCount = 0;
  let tracerBuffer = null;
  let tracerCount = 0;
  let depthTexture = null;
  let depthView = null;
  let depthWidth = 0;
  let depthHeight = 0;

  const compute = createCompute(device, domain);

  return {
    compute,
    uploadMesh(mesh) {
      const packed = packMesh(mesh);
      meshCount = mesh.length;
      if (meshBuffer) {
        meshBuffer.destroy();
      }
      meshBuffer = createBuffer(device, packed, GPUBufferUsage.VERTEX);
    },
    uploadTracers(packed) {
      tracerCount = packed.length / 6;
      if (tracerBuffer) {
        tracerBuffer.destroy();
        tracerBuffer = null;
      }
      if (tracerCount > 0) {
        tracerBuffer = createBuffer(device, packed, GPUBufferUsage.VERTEX);
      }
    },
    draw(camera, canvas) {
      const w = canvas.width;
      const h = canvas.height;
      if (w < 2 || h < 2) {
        return;
      }
      if (!depthView || w !== depthWidth || h !== depthHeight) {
        depthWidth = w;
        depthHeight = h;
        if (depthTexture) {
          depthTexture.destroy();
        }
        depthTexture = device.createTexture({
          size: { width: w, height: h },
          format: 'depth24plus',
          usage: GPUTextureUsage.RENDER_ATTACHMENT,
        });
        depthView = depthTexture.createView();
      }
      const view = viewMatrix(camera);
      const proj = projectionMatrix(w / Math.max(h, 1), camera);
      const viewProj = mul4(proj, view);
      const eye = cameraPosition(camera);
      const uniforms = new Float32Array(24);
      uniforms.set(viewProj, 0);
      uniforms.set(eye, 16);
      uniforms[19] = 1;
      uniforms.set([-0.45, 0.82, 0.35, 0], 20);
      device.queue.writeBuffer(uniformBuffer, 0, uniforms);
      const encoder = device.createCommandEncoder();
      const pass = encoder.beginRenderPass({
        colorAttachments: [{
          view: context.getCurrentTexture().createView(),
          clearValue: { r: 0.72, g: 0.82, b: 0.9, a: 1 },
          loadOp: 'clear',
          storeOp: 'store',
        }],
        depthStencilAttachment: {
          view: depthView,
          depthClearValue: 1,
          depthLoadOp: 'clear',
          depthStoreOp: 'store',
        },
      });
      pass.setBindGroup(0, bindGroup);
      pass.setPipeline(skyPipeline);
      pass.draw(3);
      pass.setPipeline(floorPipeline);
      pass.setVertexBuffer(0, floorBuffer);
      pass.draw(6);
      if (meshBuffer && meshCount > 0) {
        pass.setPipeline(carPipeline);
        pass.setVertexBuffer(0, meshBuffer);
        pass.draw(meshCount);
      }
      if (tracerBuffer && tracerCount > 0) {
        pass.setPipeline(streakPipeline);
        pass.setVertexBuffer(0, tracerBuffer);
        pass.draw(tracerCount);
      }
      pass.end();
      device.queue.submit([encoder.finish()]);
    },
    async capture(camera, width, height) {
      const w = Math.max(2, Math.floor(width));
      const h = Math.max(2, Math.floor(height));
      const color = device.createTexture({
        size: { width: w, height: h },
        format,
        usage: GPUTextureUsage.RENDER_ATTACHMENT | GPUTextureUsage.COPY_SRC,
      });
      const depth = device.createTexture({
        size: { width: w, height: h },
        format: 'depth24plus',
        usage: GPUTextureUsage.RENDER_ATTACHMENT,
      });
      const view = viewMatrix(camera);
      const proj = projectionMatrix(w / Math.max(h, 1), camera);
      const viewProj = mul4(proj, view);
      const eye = cameraPosition(camera);
      const uniforms = new Float32Array(24);
      uniforms.set(viewProj, 0);
      uniforms.set(eye, 16);
      uniforms[19] = 1;
      uniforms.set([-0.45, 0.82, 0.35, 0], 20);
      device.queue.writeBuffer(uniformBuffer, 0, uniforms);
      const bytesPerRow = Math.ceil((w * 4) / 256) * 256;
      const readBuffer = device.createBuffer({
        size: bytesPerRow * h,
        usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ,
      });
      const encoder = device.createCommandEncoder();
      const pass = encoder.beginRenderPass({
        colorAttachments: [{
          view: color.createView(),
          clearValue: { r: 0.72, g: 0.82, b: 0.9, a: 1 },
          loadOp: 'clear',
          storeOp: 'store',
        }],
        depthStencilAttachment: {
          view: depth.createView(),
          depthClearValue: 1,
          depthLoadOp: 'clear',
          depthStoreOp: 'store',
        },
      });
      pass.setBindGroup(0, bindGroup);
      pass.setPipeline(skyPipeline);
      pass.draw(3);
      pass.setPipeline(floorPipeline);
      pass.setVertexBuffer(0, floorBuffer);
      pass.draw(6);
      if (meshBuffer && meshCount > 0) {
        pass.setPipeline(carPipeline);
        pass.setVertexBuffer(0, meshBuffer);
        pass.draw(meshCount);
      }
      if (tracerBuffer && tracerCount > 0) {
        pass.setPipeline(streakPipeline);
        pass.setVertexBuffer(0, tracerBuffer);
        pass.draw(tracerCount);
      }
      pass.end();
      encoder.copyTextureToBuffer(
        { texture: color },
        { buffer: readBuffer, bytesPerRow, rowsPerImage: h },
        { width: w, height: h },
      );
      device.queue.submit([encoder.finish()]);
      await device.queue.onSubmittedWorkDone();
      await readBuffer.mapAsync(GPUMapMode.READ);
      const packed = new Uint8Array(readBuffer.getMappedRange().slice());
      readBuffer.unmap();
      readBuffer.destroy();
      color.destroy();
      depth.destroy();
      const rgba = new Uint8ClampedArray(w * h * 4);
      const bgra = format === 'bgra8unorm' || format === 'bgra8unorm-srgb';
      for (let y = 0; y < h; ++y) {
        for (let x = 0; x < w; ++x) {
          const src = y * bytesPerRow + x * 4;
          const dst = (y * w + x) * 4;
          if (bgra) {
            rgba[dst] = packed[src + 2];
            rgba[dst + 1] = packed[src + 1];
            rgba[dst + 2] = packed[src];
            rgba[dst + 3] = packed[src + 3];
          } else {
            rgba[dst] = packed[src];
            rgba[dst + 1] = packed[src + 1];
            rgba[dst + 2] = packed[src + 2];
            rgba[dst + 3] = packed[src + 3];
          }
        }
      }
      return { width: w, height: h, rgba };
    },
  };
}

function createCompute(device, domain) {
  try {
    const module = device.createShaderModule({ code: lbmWgsl });
    const count = cellCount(domain);
    const bgl = device.createBindGroupLayout({
      entries: [
        { binding: 0, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'uniform' } },
        { binding: 1, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
        { binding: 2, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'read-only-storage' } },
        { binding: 3, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
        { binding: 4, visibility: GPUShaderStage.COMPUTE, buffer: { type: 'storage' } },
      ],
    });
    const layout = device.createPipelineLayout({ bindGroupLayouts: [bgl] });
    const collide = device.createComputePipeline({
      layout,
      compute: { module, entryPoint: 'collide' },
    });
    const stream = device.createComputePipeline({
      layout,
      compute: { module, entryPoint: 'stream' },
    });
    const paramBuffer = device.createBuffer({ size: 32, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST });
    const occBuffer = device.createBuffer({
      size: count * 4,
      usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
    });
    const popA = device.createBuffer({
      size: count * LbmQ * 4,
      usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST | GPUBufferUsage.COPY_SRC,
    });
    const popB = device.createBuffer({
      size: count * LbmQ * 4,
      usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST | GPUBufferUsage.COPY_SRC,
    });
    const macroBuffer = device.createBuffer({
      size: count * 16,
      usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC,
    });
    return { collide, stream, paramBuffer, occBuffer, popA, popB, macroBuffer, bgl, ready: true };
  } catch (error) {
    return { ready: false, error };
  }
}
