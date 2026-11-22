import { defineConfig } from 'vite';

export default defineConfig({
  base: './',
  publicDir: 'public',
  server: { host: '127.0.0.1', port: 5173 },
  preview: { host: '127.0.0.1', port: 4173 },
  build: {
    target: 'es2022',
    sourcemap: true,
    rollupOptions: {
      output: {
        entryFileNames: 'assets/tunnel-m37-[hash].js',
        chunkFileNames: 'assets/[name]-m37-[hash].js',
        assetFileNames: 'assets/[name]-m37-[hash][extname]',
      },
    },
  },
});
