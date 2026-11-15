# Wind tunnel (WebGPU)

Phone-first D3Q19 lattice Boltzmann tunnel around a closed Alfa Romeo C42 triangle mesh. The desktop Vulkan C++ app is unchanged on `cursor/vulkan-showcase-3742`.

- WebGPU when the adapter works; otherwise WebGL2 (WebGL1 if needed). Same HUD and live Cp.
- JS D3Q19 is the portable solver used for occupancy rebuild, live Cp, and tests.
- Yaw / pitch sliders rebuild occupancy and re-paint live Cp (lattice + facing-into-inlet).
- Inlet smoke-rake tracers (default on) follow the live LBM after Play. Inlet in km/h. Drag to orbit, pinch to zoom. `?gl=1` forces WebGL.

```bash
cd web
npm install
npm test
npm run dev
```
