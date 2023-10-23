#pragma once

struct DepthStencilOptions {
  uint32_t enabledepthTest;
  uint32_t enabledepthWrite;
  uint32_t enabledepthBoundsTest;
  uint32_t enableStencilTest;

  DepthStencilOptions(uint32_t enabledepthTest, uint32_t enabledepthWrite,
                      uint32_t enabledepthBoundsTest,
                      uint32_t enableStencilTest)
      : enabledepthTest(enabledepthTest), enabledepthWrite(enabledepthWrite),
        enabledepthBoundsTest(enabledepthBoundsTest),
        enableStencilTest(enableStencilTest) {}

  DepthStencilOptions() {}
};