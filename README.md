# SPH Fluid

Real-time fluid simulation on the GPU: smoothed particle hydrodynamics in Vulkan compute shaders, rendered as a screen-space fluid surface.

> Work in progress. The original thesis project is being rebuilt from scratch; this branch contains the foundation (build, Vulkan context, headless rendering, tests, CI).

## Build

Needs CMake 3.25+, Ninja, a C++20 compiler, and a Vulkan 1.3 driver at runtime. Dependencies, including the glslang shader compiler, are fetched and built by CMake; no Vulkan SDK required.

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
build/release/bin/sph-fluid                          # window
build/release/bin/sph-fluid --capture out.png        # last frame
build/release/bin/sph-fluid --sequence frames --frames 12
```

In the window: right-drag orbits, middle-drag pans, scroll zooms. GPU timings and camera controls are in the overlay.

On Linux without a GPU, install Mesa's lavapipe (`mesa-vulkan-drivers`).

## Credits

Started as the thesis "Efficient rendering of SPH based fluid simulations" by András Tóth, with guidance from András Máté Fridvalszky.
