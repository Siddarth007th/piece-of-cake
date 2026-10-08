import test from 'node:test';
import assert from 'node:assert/strict';
import {createHmac} from 'node:crypto';
import {allowedOrigin, health, peerOptions, port} from '../server-utils.mjs';

test('readiness needs an identified, streaming game and an available seat', () => {
  const registry = {streamers: []};
  assert.equal(health(registry).status, 503);
  registry.streamers.push({streamerId:'unknown',streaming:true,subscribers:new Set()});
  assert.equal(health(registry).status, 503);
  registry.streamers[0].streamerId = 'piece-of-cake';
  registry.streamers[0].streaming = false;
  assert.equal(health(registry).status, 503);
  registry.streamers[0].streaming = true;
  assert.deepEqual(health(registry), {status:200,body:{ready:true}});
  registry.streamers[0].subscribers.add('player-one');
  assert.equal(health(registry).status, 409);
});

test('TURN credentials are time-limited HMAC credentials, never the shared secret', () => {
  const env = {TURN_URL:'turn:relay.example.test:3478', TURN_SECRET:'unit-test-only-secret-0123456789'};
  const config = peerOptions(env, 1000000);
  const server = config.iceServers[0];
  assert.equal(server.username, '87400:poc');
  assert.equal(server.credential, createHmac('sha1',env.TURN_SECRET).update(server.username).digest('base64'));
  assert.ok(!JSON.stringify(config).includes(env.TURN_SECRET));
  assert.notEqual(peerOptions(env,2000000).iceServers[0].credential, server.credential);
});

test('incomplete TURN settings fail instead of silently advertising a broken relay', () => {
  assert.throws(() => peerOptions({TURN_URL:'turn:example.test'}), /TURN_SECRET/);
  assert.deepEqual(peerOptions({}), {iceServers:[]});
});

test('websocket origin allowlist uses exact origins', () => {
  const origins = new Set(['https://cake.example.test']);
  assert.ok(allowedOrigin('https://cake.example.test',origins));
  for (const origin of ['https://cake.example.test.attacker.test','null',undefined,'http://cake.example.test']) assert.equal(allowedOrigin(origin,origins),false);
});

test('invalid network ports fail fast', () => {
  assert.equal(port(undefined,8080),8080);
  assert.equal(port('8888',0),8888);
  for (const value of ['oops','0','65536','80.5']) assert.throws(() => port(value,8080));
});


test('free community relay is explicit, time-limited, and can be tested without a direct-media fallback', () => {
  const options = peerOptions({FREE_RELAY:'1',ICE_RELAY_ONLY:'1'}, 1000000);
  assert.equal(options.iceTransportPolicy, 'relay');
  assert.equal(options.iceServers[0].username, '87400:poc');
  assert.ok(options.iceServers[0].urls.every(url => url.startsWith('turn:staticauth.openrelay.metered.ca:443')));
  assert.ok(!JSON.stringify(options).includes('openrelayprojectsecret'));
  assert.throws(() => peerOptions({ICE_RELAY_ONLY:'1'}), /TURN service/);
  assert.throws(() => peerOptions({FREE_RELAY:'1',TURN_URL:'turn:example.test',TURN_SECRET:'short'}), /TURN_SECRET/);
});
