import { expect, test } from '@playwright/test';

async function assertHud(page) {
  await expect(page.locator('#fallback.show')).toHaveCount(0);
  await expect(page.locator('#yaw')).toBeVisible();
  await expect(page.locator('#pitch')).toBeVisible();
  await expect(page.locator('#wind')).toBeVisible();
  await expect(page.locator('#play')).toHaveText(/Play|Pause/);
  await expect(page.locator('#tracers')).toBeVisible();
  await expect(page.locator('#hud-hide')).toBeVisible();
  await expect(page.locator('#hud-hide')).toHaveText(/Hide menu/);
  const hideBox = await page.locator('#hud-hide').boundingBox();
  const hudBox = await page.locator('#hud').boundingBox();
  expect(hideBox).toBeTruthy();
  expect(hudBox).toBeTruthy();
  expect(hideBox.y).toBeLessThan(hudBox.y + 16);
  expect(hideBox.width).toBeGreaterThan(hudBox.width * 0.7);
  const box = await page.locator('#yaw').boundingBox();
  expect(box).toBeTruthy();
  expect(box.height).toBeGreaterThan(24);
  expect(box.width).toBeGreaterThan(160);
}

test('HUD sliders are usable on this viewport', async ({ page }, testInfo) => {
  await page.goto('/');
  await page.waitForFunction(() => window.__sphReady === true, null, { timeout: 40_000 });
  await assertHud(page);
  await page.screenshot({ path: testInfo.outputPath('viewport.png') });
});

test('WebGL path runs when the WebGPU adapter is forced off', async ({ page }, testInfo) => {
  await page.goto('/?gl=1');
  await page.waitForFunction(() => window.__sphReady === true, null, { timeout: 40_000 });
  await expect.poll(() => page.evaluate(() => window.__sphApi)).toMatch(/WebGL/);
  await assertHud(page);
  await expect(page.locator('#status')).toContainText(/WebGL/i);
  await expect(page.locator('#fallback.show')).toHaveCount(0);
  await page.screenshot({ path: testInfo.outputPath('webgl.png') });
});

test('Hide collapses the HUD and Menu brings it back', async ({ page }) => {
  await page.goto('/?gl=1');
  await page.waitForFunction(() => window.__sphReady === true, null, { timeout: 40_000 });
  await assertHud(page);
  await page.locator('#hud-hide').click();
  await expect(page.locator('#yaw')).toBeHidden();
  await expect(page.locator('#hud-show')).toBeVisible();
  await expect(page.locator('#play-mini')).toBeVisible();
  await expect.poll(() => page.evaluate(() => window.__sphHud())).toBe(false);
  await page.locator('#hud-show').click();
  await expect(page.locator('#yaw')).toBeVisible();
  await expect.poll(() => page.evaluate(() => window.__sphHud())).toBe(true);
});
