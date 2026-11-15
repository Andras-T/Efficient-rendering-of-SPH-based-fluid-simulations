# Wind tunnel (WebGPU)

Phone-first D3Q19 lattice Boltzmann tunnel around a closed Alfa Romeo C42 triangle mesh. The desktop Vulkan C++ app is unchanged on `cursor/vulkan-showcase-3742`.

- WebGPU render + WGSL collide/stream shaders. The JS solver is the same D3Q19 used when computing on the CPU (tests and occupancy rebuild).
- Yaw / pitch sliders rebuild occupancy and re-paint live Cp (lattice + facing-into-inlet, not an AABB bake).
- Inlet in km/h. Drag to orbit, pinch to zoom. No hover-only probe.
- Missing WebGPU: a full-screen message, not a screenshot of the Vulkan app.

```bash
cd web
npm install
npm test
npm run dev
```
