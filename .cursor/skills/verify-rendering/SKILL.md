---
name: verify-rendering
description: Verify rendering or simulation changes without a GPU, using lavapipe headless captures, GPU tests, and an X display screenshot. Use after touching shaders, passes, or the frame loop.
---

# Verify rendering without a GPU

1. Build: `cmake --build --preset debug`.
2. GPU tests with validation: `ctest --preset debug -L gpu`. Any validation error fails a test.
3. Headless capture, then open the PNG and look at it:
   `build/debug/bin/sph-fluid --validation --frames 3 --size 1280x720 --capture /tmp/frame.png`
4. Windowed path (swapchain, ImGui), if an X server is available (cloud VMs expose `DISPLAY=:1`):
   ```bash
   DISPLAY=:1 timeout 8 build/debug/bin/sph-fluid --size 800x450 &
   sleep 4 && ffmpeg -loglevel error -y -f x11grab -i :1 -frames:v 1 /tmp/desktop.png
   ```
   `--frames N` exits after N frames for scripted runs.
5. For visual changes, compare before and after captures at the same size and frame count. Lavapipe is deterministic enough for that.

The `error: XDG_RUNTIME_DIR is invalid` line comes from GLFW probing Wayland and is harmless.
