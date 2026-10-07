#!/usr/bin/env python3
"""Original deterministic synthesis. No samples or third-party melodies."""
import math
import random
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Content/Audio/Source"
RATE = 24000


def save(name, samples):
    OUT.mkdir(parents=True, exist_ok=True)
    maximum = max(.5, max(abs(x) for x in samples))
    pcm = b"".join(struct.pack("<h", round(max(-1, min(1, x / maximum * .72)) * 32767)) for x in samples)
    with wave.open(str(OUT / f"{name}.wav"), "wb") as output:
        output.setparams((1, 2, RATE, len(samples), "NONE", "not compressed"))
        output.writeframes(pcm)


def effect(name, duration, frequency, end_frequency, noise=0):
    rng = random.Random(name)
    samples = []
    phase = 0
    for i in range(int(RATE * duration)):
        t, p = i / RATE, i / (RATE * duration)
        phase += 2 * math.pi * (frequency + (end_frequency - frequency) * p) / RATE
        envelope = min(1, t * 80) * (1 - p) ** 2
        samples.append(envelope * (.5 * math.sin(phase) + .17 * math.sin(phase * 2.01) + noise * rng.uniform(-1, 1)))
    save("A_" + name, samples)


def music(section):
    duration = 16
    data = [0.] * (RATE * duration)
    # 32 beats and a whole-number loop; different voicing and rhythm per biome.
    modes = [[0, 4, 7, 11, 12], [0, 3, 7, 10, 12], [0, 2, 7, 9, 14],
             [0, 5, 7, 10, 12], [0, 2, 5, 7, 12], [0, 4, 7, 9, 14], [0, 3, 5, 7, 10], [0, 4, 7, 12, 16]]
    rng = random.Random(6400 + section)
    for beat in range(32):
        note = modes[section][(beat * 3 + beat // 5 + section) % 5]
        frequency = 130.8128 * 2 ** (note / 12)
        start = int(beat * .5 * RATE)
        length = int((1.7 if section == 4 else 1.1) * RATE)
        for n in range(length):
            t = n / RATE
            envelope = (1 - math.exp(-t * 55)) * math.exp(-t * (2.8 if section != 4 else 1.8))
            value = (math.sin(2 * math.pi * frequency * t) + .22 * math.sin(2 * math.pi * frequency * 2 * t)) * envelope * .085
            data[(start + n) % len(data)] += value
        if (section in (1, 3, 6) and beat % (1 if section == 6 else 2) == 0):
            for n in range(int(.18 * RATE)):
                t = n / RATE
                data[(start + n) % len(data)] += .12 * math.exp(-t * 28) * (math.sin(2 * math.pi * (95 * t - 100 * t * t)) + .12 * rng.uniform(-1, 1))
    # Harmonic low bed at integer Hz makes the waveform continuous at loop edges.
    for n in range(len(data)):
        t = n / RATE
        data[n] += .035 * math.sin(2 * math.pi * 65 * t) + .02 * math.sin(2 * math.pi * 98 * t)
    save(f"M_{section}", data)


if __name__ == "__main__":
    effects = {
        "Jump": (.22, 280, 650, .02), "Land": (.12, 130, 70, .16),
        "Bonk": (.18, 190, 80, .05), "Slam": (.36, 95, 35, .2),
        "Bounce": (.4, 180, 980, .01), "Slide": (.35, 170, 110, .18),
        "Shard": (.32, 950, 1450, 0), "Relic": (.9, 440, 1100, 0),
        "Checkpoint": (.65, 380, 760, 0), "Echo": (1.6, 160, 620, .012),
        "Hurt": (.28, 330, 140, .03), "Pop": (.2, 450, 150, .1),
        "Warn": (.3, 150, 220, .05), "Rumble": (.65, 90, 35, .3),
        "Cake": (1.7, 523, 1046, 0),
    }
    for name, parameters in effects.items():
        effect(name, *parameters)
    for section in range(8):
        music(section)
    print(f"Generated {len(effects)} effects and 8 original music loops in {OUT}")
