import type {PixelStreaming} from '@epicgames-ps/lib-pixelstreamingfrontend-ue5.8';

// Opt-in, local QA display. No telemetry is sent to a remote service.
export function attachDiagnostics(client: PixelStreaming, parent: HTMLElement) {
  if (new URLSearchParams(location.search).get('diagnostics') !== '1') return;
  const panel = document.createElement('details');
  panel.id = 'stream-diagnostics';
  panel.style.cssText = 'position:fixed;right:12px;top:12px;z-index:50;background:#101c24ee;color:#fff;padding:12px;max-width:460px;max-height:65vh;overflow:auto;font:12px monospace';
  const summary = document.createElement('summary'); summary.textContent = 'Stream diagnostics';
  const test = document.createElement('button'); test.textContent = 'Measure latency';
  const save = document.createElement('button'); save.textContent = 'Save measurements';
  const output = document.createElement('pre'); output.id = 'stream-measurements'; output.style.whiteSpace = 'pre-wrap';
  panel.append(summary, test, save, output); document.body.append(panel);
  const samples: unknown[] = [], latency: unknown[] = [];
  let latest: Record<string, unknown> = {};
  function render() { output.textContent = JSON.stringify({latest, latency}, null, 2); }
  client.addEventListener('statsReceived', ({data}) => {
    const stats = data.aggregatedStats, v = stats.inboundVideoStats, a = stats.inboundAudioStats;
    const video = parent.querySelector('video');
    const quality = video?.getVideoPlaybackQuality();
    latest = {time: new Date().toISOString(), width:v.frameWidth, height:v.frameHeight,
      fps:v.framesPerSecond, decoded:v.framesDecoded, dropped:v.framesDropped,
      freezes:v.freezeCount, freezeSeconds:v.totalFreezesDuration, packetsLost:v.packetsLost,
      decodeMs:v.framesDecoded ? (v.totalDecodeTime ?? 0)*1000/v.framesDecoded : null,
      audioBytes:a.bytesReceived, audioConcealedSamples:a.concealedSamples, audioConcealmentEvents:a.concealmentEvents, audioEnergy:a.totalAudioEnergy, audioLevel:a.audioLevel,
      media:video ? {paused:video.paused, muted:video.muted, volume:video.volume,
        time:video.currentTime, quality:quality ? {totalVideoFrames:quality.totalVideoFrames, droppedVideoFrames:quality.droppedVideoFrames, corruptedVideoFrames:quality.corruptedVideoFrames} : null,
        tracks:(video.srcObject as MediaStream | null)?.getTracks().map(t=>({kind:t.kind,state:t.readyState,muted:t.muted}))} : null};
    if (samples.length < 1800) samples.push(latest);
    render();
  });
  client.addEventListener('latencyTestResult', ({data}) => { latency.push({kind:'Epic protocol response and encode timing; not motion-to-photon', ...data.latencyTimings}); render(); });
  // UE 5.8.3 advertises LatencyTest but not the newer DataChannelLatencyTest
  // opcode. Use the negotiated protocol and report its timings literally.
  let pending: ReturnType<typeof setInterval> | undefined;
  test.onclick = () => {
    clearInterval(pending); let count = 0;
    client.requestLatencyTest();
    pending = setInterval(() => { client.requestLatencyTest(); if (++count >= 9) clearInterval(pending); }, 1000);
  };
  save.onclick = () => {
    const url = URL.createObjectURL(new Blob([JSON.stringify({recorded:new Date().toISOString(),origin:location.origin,samples,latency},null,2)],{type:'application/json'}));
    const link = document.createElement('a'); link.href=url;link.download='piece-of-cake-stream-measurements.json';link.click();
    setTimeout(()=>URL.revokeObjectURL(url),1000);
  };
}
