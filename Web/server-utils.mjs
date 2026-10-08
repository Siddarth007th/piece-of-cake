import { createHmac } from 'node:crypto';

export function port(value, fallback) {
  const result = Number(value ?? fallback);
  if (!Number.isInteger(result) || result < 1 || result > 65535) throw new Error('Invalid port');
  return result;
}

export function health(registry) {
  const streamer = registry.streamers.find(item => item.streamerId === 'piece-of-cake' && item.streaming);
  if (!streamer) return {status: 503, body: {ready: false, reason: 'no_streamer'}};
  if (streamer.subscribers.size >= 1) return {status: 409, body: {ready: false, reason: 'busy'}};
  return {status: 200, body: {ready: true}};
}

export function peerOptions(env, now = Date.now(), role = 'player') {
  const iceServers = [];
  if (env.STUN_URL) iceServers.push({urls: env.STUN_URL});
  if (env.TURN_URL) {
    if (!env.TURN_SECRET || env.TURN_SECRET.length < 24) throw new Error('TURN_URL requires a TURN_SECRET of at least 24 characters');
    const lifetime = role === 'streamer' ? 31536000 : 86400;
    const username = `${Math.floor(now / 1000) + lifetime}:poc`;
    const credential = createHmac('sha1', env.TURN_SECRET).update(username).digest('base64');
    iceServers.push({urls: env.TURN_URL.split(','), username, credential});
  }
  if (env.FREE_RELAY === '1') {
    // Public static-auth service documented by the Open Relay Project. Opt-in only.
    // This published value is not an account credential; never replace custom TURN validation.
    const username = `${Math.floor(now / 1000) + 86400}:poc`;
    const credential = createHmac('sha1', 'openrelayprojectsecret').update(username).digest('base64');
    iceServers.push({urls: ['turn:staticauth.openrelay.metered.ca:443?transport=udp',
      'turn:staticauth.openrelay.metered.ca:443?transport=tcp'], username, credential});
  }
  if (env.ICE_RELAY_ONLY === '1' && !env.TURN_URL && env.FREE_RELAY !== '1') throw new Error('Relay-only testing needs a TURN service');
  return {iceServers, ...(env.ICE_RELAY_ONLY === '1' ? {iceTransportPolicy:'relay'} : {})};
}

export function allowedOrigin(origin, allowed) {
  return typeof origin === 'string' && allowed.has(origin);
}
