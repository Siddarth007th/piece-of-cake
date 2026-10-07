// Title presentation only; gameplay remains the real Unreal stream.
const el = <T extends HTMLElement>(id: string) => document.getElementById(id) as T;
const music = new Audio('/audio/menu.wav');
const effect = new Audio('/audio/select.wav');
const musicButton = el<HTMLButtonElement>('sound-toggle');
const level = el<HTMLInputElement>('menu-volume');
const motion = el<HTMLInputElement>('motion');
const systemMotion = matchMedia('(prefers-reduced-motion: reduce)');
music.loop = true; music.preload = effect.preload = 'none';
let sound = false, selected = -1, lastNavigation = 0, lastA = false, lastB = false;
try {
  const saved = localStorage.getItem('poc-title-volume');
  if (saved !== null && Number.isFinite(Number(saved))) level.value = String(Math.min(100, Math.max(0, Number(saved))));
  const preference = localStorage.getItem('poc-title-motion');
  motion.checked = preference === null ? !systemMotion.matches : preference === 'on';
} catch { motion.checked = !systemMotion.matches; }
function applyMotion() {
  document.body.classList.toggle('reduced-motion', !motion.checked);
  document.body.classList.toggle('allow-motion', motion.checked);
  if (!motion.checked) { document.documentElement.style.setProperty('--px','0px'); document.documentElement.style.setProperty('--py','0px'); }
}
function applyVolume() {
  music.volume = Number(level.value) / 100; effect.volume = music.volume * .28;
  el('menu-volume-value').textContent = `${level.value}%`;
  try { localStorage.setItem('poc-title-volume',level.value); } catch { /* Optional. */ }
}
function soundState() {
  musicButton.setAttribute('aria-pressed', String(sound));
  musicButton.setAttribute('aria-label', sound ? 'Mute menu music' : 'Enable menu music');
  el('sound-state').textContent = sound ? 'ON' : 'OFF';
}
async function playMusic() {
  if (!sound || document.hidden || document.body.classList.contains('playing')) { music.pause(); return; }
  try { await music.play(); } catch { sound = false; soundState(); }
}
applyMotion(); applyVolume(); soundState();
motion.addEventListener('change', () => { applyMotion(); try { localStorage.setItem('poc-title-motion',motion.checked ? 'on' : 'off'); } catch { /* Optional. */ } });
systemMotion.addEventListener('change', () => {
  try { if (localStorage.getItem('poc-title-motion') !== null) return; } catch { /* Use system preference. */ }
  motion.checked = !systemMotion.matches; applyMotion();
});
level.addEventListener('input', () => { applyVolume(); sound = music.volume > 0; soundState(); void playMusic(); });
musicButton.addEventListener('click', () => { sound = !sound; soundState(); void playMusic(); });
new MutationObserver(() => { void playMusic(); }).observe(document.body,{attributes:true,attributeFilter:['class']});
document.addEventListener('visibilitychange', () => { void playMusic(); });
window.addEventListener('pagehide', () => music.pause());
el('settings-open').addEventListener('click', () => el<HTMLDialogElement>('settings-dialog').showModal());
el('setup-open').addEventListener('click', () => el<HTMLDialogElement>('setup-dialog').showModal());
el('title-fullscreen').addEventListener('click', async () => {
  try { if (document.fullscreenElement) await document.exitFullscreen(); else await document.documentElement.requestFullscreen(); }
  catch { const box=el('notice'); box.textContent='Fullscreen is unavailable in this browser.'; box.hidden=false; setTimeout(() => { box.hidden=true; },4500); }
});
document.addEventListener('fullscreenchange', () => el('title-fullscreen').setAttribute('aria-label',document.fullscreenElement ? 'Exit fullscreen' : 'Enter fullscreen'));
const buttons = Array.from(document.querySelectorAll<HTMLButtonElement>('.game-menu button'));
buttons.forEach((button,index) => {
  button.addEventListener('focus', () => { selected=index; });
  button.addEventListener('mouseenter', () => {
    if (button.disabled || document.querySelector('dialog[open]')) return;
    selected=index;
    if (sound) { effect.currentTime=0; void effect.play().catch(() => {}); }
  });
});
function navigate(direction:number) {
  const dialog=document.querySelector<HTMLDialogElement>('dialog[open]');
  if (dialog) {
    const targets=Array.from(dialog.querySelectorAll<HTMLElement>('button:not(:disabled),input'));
    const index=targets.indexOf(document.activeElement as HTMLElement);
    targets[(index+direction+targets.length)%targets.length]?.focus(); return;
  }
  for(let tries=0;tries<buttons.length;tries++) {
    selected=(selected+direction+buttons.length)%buttons.length;
    if(!buttons[selected].disabled) { buttons[selected].focus(); break; }
  }
}
document.addEventListener('keydown',event => {
  if(document.body.classList.contains('playing') || document.querySelector('dialog[open]')) return;
  if(event.key==='ArrowDown' || event.key==='ArrowUp') { event.preventDefault(); navigate(event.key==='ArrowDown'?1:-1); }
  if(event.key==='Enter' && document.activeElement===document.body) { event.preventDefault(); el<HTMLButtonElement>('play').click(); }
});
window.addEventListener('pointermove',event => {
  if(!motion.checked || event.pointerType==='touch' || document.body.classList.contains('playing')) return;
  document.documentElement.style.setProperty('--px',`${(event.clientX/innerWidth-.5)*16}px`);
  document.documentElement.style.setProperty('--py',`${(event.clientY/innerHeight-.5)*10}px`);
},{passive:true});
const canvas=el<HTMLCanvasElement>('fireflies'), context=canvas.getContext('2d');
const particles=Array.from({length:32},(_,i)=>({x:((i*73+17)%101)/101,y:((i*43+13)%97)/97,size:1+i%3,speed:.006+i%5*.002}));
function resize() {
  const ratio=Math.min(devicePixelRatio,1.5); canvas.width=Math.round(innerWidth*ratio); canvas.height=Math.round(innerHeight*ratio);
  context?.setTransform(ratio,0,0,ratio,0,0);
}
resize(); window.addEventListener('resize',resize);
let lastPaint=0;
function tick(now:number) {
  requestAnimationFrame(tick);
  if(document.hidden || document.body.classList.contains('playing') || now-lastPaint<32) return;
  lastPaint=now;
  if(context) {
    context.clearRect(0,0,innerWidth,innerHeight);
    if(motion.checked) for(const [i,p] of particles.entries()) {
      const t=now/1000,x=p.x*innerWidth+Math.sin(t*.3+i)*28,y=((p.y-t*p.speed)%1+1)%1*innerHeight;
      context.fillStyle=`rgba(226,255,184,${.25+(Math.sin(t*1.2+i)+1)*.22})`; context.shadowColor='#b9ffc6'; context.shadowBlur=9;
      context.beginPath(); context.arc(x,y,p.size,0,Math.PI*2); context.fill();
    }
  }
  const pad=typeof navigator.getGamepads==='function' ? Array.from(navigator.getGamepads()).find(p=>p?.connected) : null;
  el('gamepad-hint').hidden=!pad;
  if(!pad) { lastA=lastB=false; return; }
  const down=pad.buttons[13]?.pressed || (pad.axes[1]??0)>.6, up=pad.buttons[12]?.pressed || (pad.axes[1]??0)<-.6;
  if((down||up) && now-lastNavigation>220) { navigate(down?1:-1); lastNavigation=now; }
  const a=Boolean(pad.buttons[0]?.pressed), b=Boolean(pad.buttons[1]?.pressed);
  if(a&&!lastA) {
    const target=document.activeElement as HTMLElement;
    if(target.matches('button:not(:disabled),input[type=checkbox]')) target.click();
    else if(!document.querySelector('dialog[open]')) el<HTMLButtonElement>('play').click();
  }
  if(b&&!lastB) document.querySelector<HTMLDialogElement>('dialog[open]')?.close();
  const target=document.activeElement;
  if(target instanceof HTMLInputElement && target.type==='range' && now-lastNavigation>150) {
    const direction=pad.buttons[15]?.pressed?1:pad.buttons[14]?.pressed?-1:0;
    if(direction) { target.value=String(Math.max(0,Math.min(100,Number(target.value)+direction*5))); target.dispatchEvent(new Event('input',{bubbles:true})); lastNavigation=now; }
  }
  lastA=a; lastB=b;
}
requestAnimationFrame(tick);
