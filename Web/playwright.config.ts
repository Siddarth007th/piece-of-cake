import {defineConfig} from '@playwright/test';
export default defineConfig({
  testDir: './tests/browser',
  fullyParallel: false,
  use: {baseURL:'http://127.0.0.1:18089', headless:true, screenshot:'only-on-failure'},
  projects: [{name:'chrome',use:{browserName:'chromium',channel:'chrome'}}],
  webServer: {
    command: 'npm run build && node server.mjs',
    url: 'http://127.0.0.1:18089/healthz',
    env: {PLAYER_PORT:'18089', STREAMER_PORT:'18889', PUBLIC_ORIGIN:'http://127.0.0.1:18089'},
    reuseExistingServer:false,
  },
});
