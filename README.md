# SPH Fluid

Real-time fluid simulation on the GPU: smoothed particle hydrodynamics in Vulkan compute shaders, rendered as a screen-space fluid surface.

![Dam break](docs/showcase.gif)

WCSPH by default (Tait EOS, poly6 / spiky / viscosity Laplacian) on a uniform grid, with DFSPH as a toggle. Solid walls use Akinci boundary particles. Wet foam pads spawn on crests. Screen-space fluid rendering: eye-depth impostors, additive thickness, bilateral smoothing, HDRI refraction/reflection, a faint meniscus at the glass, and a Poly Haven HDRI.

## Build

Needs CMake 3.25+, Ninja, a C++20 compiler, and a Vulkan 1.3 driver at runtime. Dependencies, including the glslang shader compiler, are fetched and built by CMake; no Vulkan SDK required.

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
build/release/bin/sph-fluid                          # window
build/release/bin/sph-fluid --scene drop             # empty, dam, drop, or double
build/release/bin/sph-fluid --view depth             # stage explorer
build/release/bin/sph-fluid --solver dfsph           # incompressible DFSPH
build/release/bin/sph-fluid --capture out.png        # last frame
build/release/bin/sph-fluid --sequence frames --frames 12
cmake --install build/release --prefix dist          # dist/bin/sph-fluid
```

Starts paused so viscosity and other parameters can be set before Play. Scene changes and Reset pause again.

Left-drag is always the paddle: a small cyan sphere sits in the tank on the mouse ray, shoves water aside, and can slide the wooden block even while paused. Uncheck Wooden block to hide it (no draw, no collision). Right-drag orbits; Shift+left or middle-drag pans; scroll zooms; WASD/QE moves; arrows look; F frames the tank. Space play/pauses. `[` `]` cycles stages (foam is last); 1–9 pick the first nine. A never changes the view.

Walls: Hidden, Glass (default closed aquarium cube on the realistic view), or Particles (Akinci spheres). The overlay also has scene presets, WCSPH/DFSPH, viscosity (default 0.033), paddle size/swirl, Reset, exposure, and GPU timings. Fluid stays inside the glass; walls are slip so splash does not glue to the lid.

On Linux without a GPU, install Mesa's lavapipe (`mesa-vulkan-drivers`).

## Credits

Started as the thesis "Efficient rendering of SPH based fluid simulations" by András Tóth, with guidance from András Máté Fridvalszky.

Environment lighting uses [Kloofendal 48d Partly Cloudy Puresky](https://polyhaven.com/a/kloofendal_48d_partly_cloudy_puresky) from [Poly Haven](https://polyhaven.com), licensed [CC0](https://creativecommons.org/publicdomain/zero/1.0/).
