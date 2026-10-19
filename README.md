# SPH Fluid

Real-time fluid simulation on the GPU: smoothed particle hydrodynamics in Vulkan compute shaders, rendered as a screen-space fluid surface.

![Dam break](docs/showcase.gif)

WCSPH (Tait EOS, poly6 / spiky / viscosity Laplacian) on a uniform grid, then screen-space fluid rendering: eye-depth impostors, additive thickness, narrow-range smoothing, Schlick Fresnel, Beer–Lambert absorption, and a Poly Haven HDRI.

## Build

Needs CMake 3.25+, Ninja, a C++20 compiler, and a Vulkan 1.3 driver at runtime. Dependencies, including the glslang shader compiler, are fetched and built by CMake; no Vulkan SDK required.

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
build/release/bin/sph-fluid                          # window
build/release/bin/sph-fluid --scene drop             # empty, dam, drop, or double
build/release/bin/sph-fluid --view depth             # stage explorer
build/release/bin/sph-fluid --capture out.png        # last frame
build/release/bin/sph-fluid --sequence frames --frames 12
cmake --install build/release --prefix dist          # dist/bin/sph-fluid
```

In the window: left-drag stirs the fluid, right-drag orbits, middle-drag pans, scroll zooms. Space pauses. `[` `]` and 1–9 step through the stage explorer. The overlay has scene presets, a viscosity slider, Reset, exposure, and GPU timings.

On Linux without a GPU, install Mesa's lavapipe (`mesa-vulkan-drivers`).

## Credits

Started as the thesis "Efficient rendering of SPH based fluid simulations" by András Tóth, with guidance from András Máté Fridvalszky.

Environment lighting uses [Kloofendal 48d Partly Cloudy Puresky](https://polyhaven.com/a/kloofendal_48d_partly_cloudy_puresky) from [Poly Haven](https://polyhaven.com), licensed [CC0](https://creativecommons.org/publicdomain/zero/1.0/).
