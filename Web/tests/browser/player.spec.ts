import {test, expect} from '@playwright/test';

test('offline service never pretends a game is playable', async ({page}) => {
  await page.goto('/');
  await expect(page.getByText('Game server offline · Check back soon')).toBeVisible();
  await page.getByRole('button',{name:'Play the adventure'}).click();
  await expect(page.getByRole('dialog',{name:'The trail is resting.'})).toBeVisible();
  await expect(page.getByText('The game server is not running yet. Please try again when it’s available.')).toBeVisible();
  await expect(page.locator('video')).toHaveCount(0);
  await page.getByRole('button',{name:'Try again'}).click();
  await expect(page.getByRole('dialog',{name:'The trail is resting.'})).toBeVisible();
  await page.keyboard.press('Escape');
  await expect(page.getByRole('dialog')).not.toBeVisible();
});

test('the keyboard and controller field guide is usable', async ({page}) => {
  await page.goto('/');
  await page.getByRole('button',{name:'How to play'}).click();
  await expect(page.getByRole('dialog',{name:'Your paws know the way.'})).toBeVisible();
  await expect(page.getByText('Jump · hold for height')).toBeVisible();
  await page.getByRole('button',{name:'Close controls'}).click();
  await expect(page.getByRole('dialog')).not.toBeVisible();
});

test('small viewport retains usable controls without horizontal overflow', async ({page}) => {
  await page.setViewportSize({width:390,height:844});
  await page.goto('/');
  await expect(page.getByRole('button',{name:'Play the adventure'})).toBeVisible();
  const widths = await page.evaluate(() => ({scroll:document.documentElement.scrollWidth,viewport:window.innerWidth}));
  expect(widths.scroll).toBeLessThanOrEqual(widths.viewport);
  await page.getByRole('button',{name:'How to play'}).click();
  await expect(page.getByRole('button',{name:'Close controls'})).toBeVisible();
});
