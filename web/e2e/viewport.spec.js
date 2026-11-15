import { expect, test } from '@playwright/test';

test('HUD sliders are usable on this viewport', async ({ page }, testInfo) => {
  await page.goto('/');
  await expect(page.getByRole('heading', { name: /wind tunnel|webgpu required/i })).toBeVisible();
  const fallback = page.locator('#fallback.show');
  if (await fallback.isVisible()) {
    await expect(page.locator('#fallback-text')).toContainText(/WebGPU|Safari|Chrome/i);
    await page.screenshot({
      path: testInfo.outputPath('viewport.png'),
      fullPage: true,
    });
    return;
  }
  await expect(page.locator('#yaw')).toBeVisible();
  await expect(page.locator('#pitch')).toBeVisible();
  await expect(page.locator('#wind')).toBeVisible();
  await expect(page.locator('#play')).toHaveText(/Play|Pause/);
  const yaw = page.locator('#yaw');
  const box = await yaw.boundingBox();
  expect(box).toBeTruthy();
  expect(box.height).toBeGreaterThan(24);
  expect(box.width).toBeGreaterThan(160);
  await page.waitForFunction(() => {
    const status = document.querySelector('#status')?.textContent ?? '';
    return status.length > 0 && !status.startsWith('Loading');
  });
  await page.screenshot({
    path: testInfo.outputPath('viewport.png'),
    fullPage: true,
  });
});
