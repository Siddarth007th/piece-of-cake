// Protocol-level fixtures below are NOT Unreal streams and never send media.
import test from 'node:test';
import assert from 'node:assert/strict';
import {spawn} from 'node:child_process';
import {once} from 'node:events';
import WebSocket from 'ws';

test('real signalling server: offline health, protocol identification, disconnection, origin rejection', {timeout:15000}, async () => {
  const server = spawn(process.execPath,['server.mjs'], {cwd:new URL('..', import.meta.url), env:{...process.env,PLAYER_PORT:'18080',STREAMER_PORT:'18888',PUBLIC_ORIGIN:'http://127.0.0.1:18080'}, stdio:['ignore','pipe','pipe']});
  const peers = [];
  let logs = '';
  server.stdout.on('data', chunk => logs += chunk);
  server.stderr.on('data', chunk => logs += chunk);
  try {
    for (let attempt=0; attempt<70; attempt++) {
      if (server.exitCode !== null) throw new Error(logs);
      try { if ((await fetch('http://127.0.0.1:18080/healthz')).ok) break; } catch {}
      await new Promise(resolve => setTimeout(resolve,50));
    }
    let response = await fetch('http://127.0.0.1:18080/readyz');
    assert.equal(response.status,503);
    assert.deepEqual(await response.json(),{ready:false,reason:'no_streamer'});
    const denied = new WebSocket('ws://127.0.0.1:18080/signal',{origin:'https://unauthorized.example.test'});
    const denial = await new Promise(resolve => denied.on('unexpected-response',(_, res)=>{res.resume();resolve(res.statusCode)}).on('error',()=>{}));
    denied.terminate();
    assert.equal(denial,401);
    const fixture = new WebSocket('ws://127.0.0.1:18888'); peers.push(fixture);
    fixture.on('message', raw => {
      const message = JSON.parse(String(raw));
      if (message.type === 'identify') fixture.send(JSON.stringify({type:'endpointId',id:'piece-of-cake'}));
    });
    await once(fixture,'open');
    for (let i=0;i<40;i++) {
      response = await fetch('http://127.0.0.1:18080/readyz');
      if (response.status === 200) break;
      await new Promise(resolve=>setTimeout(resolve,25));
    }
    assert.equal(response.status,200,'Identified protocol fixture should advertise a seat; this does not verify media');
    const first = new WebSocket('ws://127.0.0.1:18080/signal',{origin:'http://127.0.0.1:18080'}); peers.push(first);
    await once(first,'open');
    first.send(JSON.stringify({type:'subscribe',streamerId:'piece-of-cake'}));
    for (let i=0;i<40;i++) {
      response = await fetch('http://127.0.0.1:18080/readyz');
      if (response.status === 409) break;
      await new Promise(resolve=>setTimeout(resolve,25));
    }
    assert.equal(response.status,409,'The first subscribed peer occupies the only seat');
    const second = new WebSocket('ws://127.0.0.1:18080/signal',{origin:'http://127.0.0.1:18080'}); peers.push(second);
    const rejected = new Promise(resolve => second.on('message',raw=>{
      const message=JSON.parse(String(raw)); if (message.type==='subscribeFailed') resolve(message);
    }));
    await once(second,'open'); second.send(JSON.stringify({type:'subscribe',streamerId:'piece-of-cake'}));
    assert.match((await rejected).message,/full/i,'Second peer must not share control of the same game');
    first.close(); await once(first,'close');
    fixture.close(); await once(fixture,'close');
    response = await fetch('http://127.0.0.1:18080/readyz');
    assert.equal(response.status,503);
    response = await fetch('http://127.0.0.1:18080/server.mjs');
    assert.equal(response.status,404,'Server source must never be served from the web root');
  } finally {
    for (const peer of peers) peer.terminate();
    server.kill('SIGTERM');
    if (server.exitCode === null) await once(server,'exit');
  }
});
