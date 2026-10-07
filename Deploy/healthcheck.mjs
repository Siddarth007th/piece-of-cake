#!/usr/bin/env node
const url = process.argv[2] || 'http://127.0.0.1:8080';
let failed = false;
for (const endpoint of ['/healthz','/readyz']) {
  try {
    const response = await fetch(new URL(endpoint,url),{signal:AbortSignal.timeout(8000)});
    const result = await response.json();
    console.log(`${endpoint}: HTTP ${response.status}`, result);
    if (endpoint === '/readyz' && response.status === 409) console.log('The game is connected but occupied. This is not a free-seat readiness pass.');
    if (!response.ok) failed = true;
  } catch (error) { console.error(`${endpoint}: ${error.message}`); failed = true; }
}
if (!failed) console.log('HTTP and streamer registration are ready. A browser media/input playthrough is still required.');
process.exitCode = failed ? 1 : 0;
