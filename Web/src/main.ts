import { Config, Flags, NumericParameters, PixelStreaming, TextParameters } from '@epicgames-ps/lib-pixelstreamingfrontend-ue5.8';
import './style.css';

const element = <T extends HTMLElement>(id: string) => {
  const value = document.getElementById(id);
  if (!value) throw new Error(`Missing UI element: ${id}`);
  return value as T;
};
const playButton = element<HTMLButtonElement>('play');
const playerPanel = element('player-panel');
const streamParent = element('stream');
const overlay = element('stream-overlay');
const errorDialog = element<HTMLDialogElement>('error-dialog');
const controlsDialog = element<HTMLDialogElement>('controls-dialog');
const resumeButton = element<HTMLButtonElement>('resume-video');
const restartButton = element<HTMLButtonElement>('restart');
const volumeInput = element<HTMLInputElement>('volume');
const availability = element('availability');
let stream: PixelStreaming | undefined;
let connecting = false;
let playing = false;
let intentionalDisconnect = false;
let connectionTimer: ReturnType<typeof setTimeout> | undefined;
let generation = 0;

function notice(message: string) {
  const box = element('notice');
  box.textContent = message; box.hidden = false;
  window.setTimeout(() => { box.hidden = true; }, 4500);
}

function volume() {
  const amount = Math.min(1, Math.max(0, Number(volumeInput.value) / 100));
  streamParent.querySelectorAll<HTMLMediaElement>('video,audio').forEach(media => {
    media.volume = amount;
    media.muted = amount === 0;
  });
  try { localStorage.setItem('poc-volume', String(amount)); } catch { /* Private storage may be unavailable. */ }
}
try {
  const stored = localStorage.getItem('poc-volume');
  if (stored !== null && Number.isFinite(Number(stored))) volumeInput.value = String(Math.round(Math.min(1, Math.max(0, Number(stored))) * 100));
} catch { /* Use default volume. */ }
volumeInput.addEventListener('input', volume);
new MutationObserver(volume).observe(streamParent, {childList: true, subtree: true});

async function readiness() {
  const response = await fetch('/readyz', {cache: 'no-store', signal: AbortSignal.timeout(5000)});
  const body = await response.json();
  if (response.status === 409 || body.reason === 'busy') return 'busy';
  return response.ok && body.ready === true ? 'ready' : 'offline';
}

async function checkAvailability() {
  try {
    const status = await readiness();
    availability.dataset.ready = String(status === 'ready');
    element('availability-text').textContent = status === 'ready' ? 'The trail is open. Your adventure is ready.' : status === 'busy' ? 'An adventurer is on the trail. Try again shortly.' : 'Game server offline · Check back soon';
  } catch {
    availability.dataset.ready = 'false';
    element('availability-text').textContent = 'Game server unavailable · Check back soon';
  }
}

function leave() {
  ++generation;
  intentionalDisconnect = true;
  clearTimeout(connectionTimer);
  stream?.disconnect();
  connecting = playing = false;
  playerPanel.hidden = true;
  document.body.classList.remove('playing');
  restartButton.disabled = true;
  playButton.disabled = false;
  if (document.pointerLockElement) document.exitPointerLock();
  if (document.fullscreenElement) void document.exitFullscreen();
}

function fail(message: string) {
  if (!connecting && !playing) return;
  leave();
  element('error-message').textContent = message;
  if (!errorDialog.open) errorDialog.showModal();
  void checkAvailability();
}

function loading(title: string, detail: string) {
  overlay.hidden = false;
  element('stream-status').textContent = title;
  element('stream-detail').textContent = detail;
}

function createStream() {
  const url = new URL('/signal', window.location.href);
  url.protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const config = new Config({ useUrlParams: false, initialSettings: {
    [TextParameters.SignallingServerUrl]: url.href,
    [Flags.AutoConnect]: false,
    [Flags.AutoPlayVideo]: true,
    [Flags.StartVideoMuted]: false,
    [Flags.KeyboardInput]: true,
    [Flags.MouseInput]: true,
    [Flags.GamepadInput]: true,
    [Flags.TouchInput]: false,
    [Flags.HoveringMouseMode]: false,
    [Flags.SuppressBrowserKeys]: true,
    [Flags.UseMic]: false,
    [Flags.UseCamera]: false,
    [NumericParameters.MaxReconnectAttempts]: 0,
  }});
  const client = new PixelStreaming(config, {videoElementParent: streamParent});
  client.addEventListener('webRtcConnecting', () => loading('Crossing the last bridge…', 'Connecting to the live game.'));
  client.addEventListener('webRtcConnected', () => loading('Your adventure is arriving.', 'Waiting for the first live frame.'));
  client.addEventListener('videoInitialized', volume);
  client.addEventListener('playStream', () => {
    clearTimeout(connectionTimer);
    overlay.hidden = true;
    connecting = false; playing = true;
    restartButton.disabled = false;
    volume(); streamParent.focus();
  });
  client.addEventListener('playStreamRejected', () => {
    clearTimeout(connectionTimer);
    loading('One last little click.', 'Your browser needs permission to start the video and sound.');
    resumeButton.hidden = false;
  });
  client.addEventListener('playStreamError', () => fail('The live video could not start. Try another supported browser or reconnect.'));
  client.addEventListener('webRtcFailed', () => fail('We couldn’t reach the game stream. Please try again; a restricted network may be blocking the connection.'));
  client.addEventListener('webRtcDisconnected', () => { if (!intentionalDisconnect) fail('The connection to the game was lost. Reconnect to return to the adventure.'); });
  client.addEventListener('subscribeFailed', () => fail('This adventure is currently occupied or unavailable. Please try again in a moment.'));
  return client;
}

async function start() {
  if (connecting || playing) return;
  const attempt = ++generation;
  errorDialog.close();
  playButton.disabled = true;
  connecting = true; intentionalDisconnect = false;
  playerPanel.hidden = false;
  document.body.classList.add('playing');
  resumeButton.hidden = true;
  loading('Looking for the trail…', 'Checking that your adventure is ready.');
  try {
    const status = await readiness();
    if (attempt !== generation) return;
    if (status !== 'ready') {
      fail(status === 'busy' ? 'An adventurer is already on the trail. Please try again shortly.' : 'The game server is not running yet. Please try again when it’s available.');
      return;
    }
    stream ??= createStream();
    loading('Connecting to your adventure…', 'A real world takes a moment to reach.');
    connectionTimer = setTimeout(() => fail('The game stream took too long to arrive. Please reconnect or try a different network.'), 30000);
    stream.connect();
  } catch {
    if (attempt === generation) fail('The game server could not be reached. Check your connection and try again.');
  }
}

playButton.addEventListener('click', () => void start());
element('retry').addEventListener('click', () => void start());
element('cancel-connect').addEventListener('click', leave);
element('leave').addEventListener('click', leave);
resumeButton.addEventListener('click', () => { resumeButton.hidden = true; stream?.play(); volume(); });
element('controls-open').addEventListener('click', () => controlsDialog.showModal());
document.querySelectorAll<HTMLButtonElement>('[data-close]').forEach(button => button.addEventListener('click', () => element<HTMLDialogElement>(button.dataset.close!).close()));
element('fullscreen').addEventListener('click', async () => {
  try {
    if (document.fullscreenElement) await document.exitFullscreen();
    else await element('stream-frame').requestFullscreen();
  } catch { notice('Fullscreen is unavailable in this browser.'); }
});
document.addEventListener('fullscreenchange', () => element('fullscreen').setAttribute('aria-label', document.fullscreenElement ? 'Exit fullscreen' : 'Enter fullscreen'));
restartButton.addEventListener('click', () => {
  if (!playing || !stream) return;
  const down = stream.toStreamerHandlers.get('KeyDown');
  const up = stream.toStreamerHandlers.get('KeyUp');
  if (!down || !up) { notice('Press R in the game to restart your checkpoint.'); return; }
  down([82, 0]);
  setTimeout(() => up([82]), 80);
  streamParent.focus();
});
// Toolbar controls must not send their keystrokes into the game.
document.addEventListener('focusin', event => {
  const target = event.target as HTMLElement;
  const editingUI = target.closest('.stream-toolbar,dialog') !== null;
  stream?.config.setFlagEnabled(Flags.KeyboardInput, !editingUI);
});
window.addEventListener('pagehide', leave);
void checkAvailability();
