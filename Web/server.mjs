import http from 'node:http';
import {createReadStream, existsSync} from 'node:fs';
import {stat} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {SignallingServer} from '@epicgames-ps/lib-pixelstreamingsignalling-ue5.8';
import {allowedOrigin, health, peerOptions, port} from './server-utils.mjs';

const directory = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(process.env.WEB_ROOT || path.join(directory, 'dist'));
const host = process.env.BIND_HOST || '127.0.0.1';
const playerPort = port(process.env.PLAYER_PORT, 8080);
const streamerPort = port(process.env.STREAMER_PORT, 8888);
const production = process.env.NODE_ENV === 'production';
if (production && !process.env.PUBLIC_ORIGIN) throw new Error('PUBLIC_ORIGIN is required in production');
const origins = new Set((process.env.PUBLIC_ORIGIN || `http://127.0.0.1:${playerPort},http://localhost:${playerPort},http://localhost:5173,http://127.0.0.1:5173`).split(',').map(x => x.trim()));
peerOptions(process.env); // Fail fast on invalid TURN configuration.
const mime = {'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.svg':'image/svg+xml','.woff2':'font/woff2','.json':'application/json'};
let signalling;

function json(response, status, body) {
  response.writeHead(status, {'content-type':'application/json','cache-control':'no-store'});
  response.end(JSON.stringify(body));
}

const server = http.createServer(async (request, response) => {
  response.setHeader('X-Content-Type-Options', 'nosniff');
  response.setHeader('Referrer-Policy', 'same-origin');
  response.setHeader('Permissions-Policy', 'camera=(), microphone=(), geolocation=()');
  response.setHeader('Content-Security-Policy', "default-src 'self'; script-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; connect-src 'self' ws: wss:; media-src 'self' blob:; font-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'self'");
  if (!['GET','HEAD'].includes(request.method)) return json(response, 405, {error:'method_not_allowed'});
  try {
    const url = new URL(request.url, 'http://localhost');
    if (url.pathname === '/healthz') return json(response, 200, {service:'piece-of-cake', alive:true, engine:'5.8'});
    if (url.pathname === '/readyz') {
      const current = health(signalling.streamerRegistry);
      return json(response, current.status, current.body);
    }
    const relative = url.pathname === '/' ? '/index.html' : decodeURIComponent(url.pathname);
    const filename = path.resolve(root, '.' + relative);
    if (!filename.startsWith(root + path.sep) || !existsSync(filename)) return json(response, 404, {error:'not_found'});
    const info = await stat(filename);
    if (!info.isFile()) return json(response, 404, {error:'not_found'});
    response.writeHead(200, {'content-type':mime[path.extname(filename)] || 'application/octet-stream', 'content-length':info.size,
      'cache-control':url.pathname.startsWith('/assets/') ? 'public,max-age=31536000,immutable' : 'no-cache'});
    if (request.method === 'HEAD') return response.end();
    const file = createReadStream(filename);
    file.on('error', () => response.destroy());
    file.pipe(response);
  } catch { if (!response.headersSent) json(response, 400, {error:'bad_request'}); else response.destroy(); }
});

signalling = new SignallingServer({
  httpServer: server,
  streamerPort,
  streamerWsOptions: {host: '127.0.0.1', maxPayload: 1024 * 1024},
  playerWsOptions: {path: '/signal', maxPayload: 1024 * 1024,
    verifyClient: info => allowedOrigin(info.origin, origins)},
  peerOptions: peerOptions(process.env),
  peerOptionsProvider: request => peerOptions(process.env, Date.now(), request.peerType),
  maxSubscribers: 1,
  playerKeepaliveTimeout: 30000,
  authorizeStreamerId: ({requestedId, collided}) => requestedId === 'piece-of-cake' && !collided ? requestedId : null,
});

server.listen(playerPort, host, () => console.log(JSON.stringify({event:'listening', host, playerPort, streamerPort, webRoot:root})));
server.on('error', error => { console.error(error.message); process.exit(1); });
for (const signal of ['SIGTERM','SIGINT']) process.on(signal, () => {
  console.log(JSON.stringify({event:'shutdown', signal}));
  server.close();
  process.exit(0);
});
