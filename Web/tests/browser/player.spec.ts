import {test, expect} from '@playwright/test';

test('offline menu explains the blocker without pretending gameplay exists', async ({page}) => {
  await page.goto('/');
  await expect(page.getByRole('button',{name:'START GAME',exact:true})).toBeDisabled();
  await expect(page.getByText('Game offline · Not playable yet')).toBeVisible();
  await page.getByRole('button',{name:'Why is the game offline?'}).click();
  await expect(page.getByRole('dialog',{name:'NOT READY YET'})).toBeVisible();
  await expect(page.getByText('Play Piece of Cake.command',{exact:true})).toBeVisible();
  await expect(page.locator('video')).toHaveCount(0);
});

test('keyboard menu navigation opens a usable control guide', async ({page}) => {
  await page.goto('/');
  await expect(page.getByRole('button',{name:'START GAME',exact:true})).toBeDisabled();
  await page.keyboard.press('ArrowDown');
  await expect(page.getByRole('button',{name:'OPTIONS',exact:true})).toBeFocused();
  await page.keyboard.press('ArrowDown');
  await page.keyboard.press('Enter');
  await expect(page.getByRole('dialog',{name:'HOW TO PLAY'})).toBeVisible();
  await expect(page.getByText('Jump · hold for height')).toBeVisible();
  await page.keyboard.press('Escape');
  await expect(page.getByRole('dialog')).not.toBeVisible();
});

test('motion and volume preferences survive reload', async ({page}) => {
  await page.goto('/');
  await page.getByRole('button',{name:'OPTIONS',exact:true}).click();
  await page.getByLabel('Animate title screen').uncheck();
  const slider=page.getByRole('slider',{name:'Menu music'});
  await slider.focus();
  await page.keyboard.press('Home');
  await page.keyboard.press('ArrowRight');
  await expect(slider).toHaveValue('1');
  await page.reload();
  await page.getByRole('button',{name:'OPTIONS',exact:true}).click();
  await expect(page.getByLabel('Animate title screen')).not.toBeChecked();
  await expect(page.getByRole('slider',{name:'Menu music'})).toHaveValue('1');
  await expect(page.locator('body')).toHaveClass(/reduced-motion/);
});

for(const viewport of [{width:1470,height:875},{width:390,height:844},{width:844,height:390}]) {
  test(`title and start remain on one screen at ${viewport.width}x${viewport.height}`,async ({page})=>{
    await page.setViewportSize(viewport);
    await page.goto('/');
    await expect(page.getByRole('heading',{name:'Piece of Cake',exact:true})).toBeVisible();
    const start=page.getByRole('button',{name:'START GAME',exact:true});
    await expect(start).toBeVisible();
    const bounds=await start.boundingBox();
    expect(bounds).not.toBeNull();
    expect(bounds!.y+bounds!.height).toBeLessThanOrEqual(viewport.height);
    const size=await page.evaluate(()=>({w:document.documentElement.scrollWidth,h:document.documentElement.scrollHeight}));
    expect(size.w).toBeLessThanOrEqual(viewport.width);
    expect(size.h).toBeLessThanOrEqual(viewport.height);
  });
}
