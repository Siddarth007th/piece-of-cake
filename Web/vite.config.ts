import { defineConfig } from 'vite';
export default defineConfig({
  server: {
    strictPort: true,
    proxy: {
      '/healthz': 'http://127.0.0.1:8080',
      '/readyz': 'http://127.0.0.1:8080',
      '/signal': { target: 'ws://127.0.0.1:8080', ws: true },
    },
  },
  build: { target: 'es2022' },
});
