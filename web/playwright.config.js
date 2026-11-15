import { defineConfig, devices } from '@playwright/test';

const chromeArgs = [
  '--enable-unsafe-webgpu',
  '--enable-features=Vulkan,WebGPU',
  '--use-angle=vulkan',
  '--ignore-gpu-blocklist',
];

export default defineConfig({
  testDir: './e2e',
  timeout: 60_000,
  use: {
    baseURL: 'http://127.0.0.1:5173',
    trace: 'off',
    launchOptions: { args: chromeArgs },
  },
  webServer: {
    command: 'npx vite --host 127.0.0.1 --port 5173',
    url: 'http://127.0.0.1:5173',
    reuseExistingServer: true,
    timeout: 60_000,
  },
  projects: [
    {
      name: 'phone',
      use: {
        browserName: 'chromium',
        viewport: { width: 390, height: 844 },
        isMobile: true,
        hasTouch: true,
        deviceScaleFactor: 3,
        userAgent: devices['iPhone 13'].userAgent,
      },
    },
    {
      name: 'desktop',
      use: { browserName: 'chromium', viewport: { width: 1280, height: 720 } },
    },
  ],
});
