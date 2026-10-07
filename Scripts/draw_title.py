#!/usr/bin/env python3
"""Original SVG title illustration, never presented as a gameplay screenshot."""
import random
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "Web/public/journey.svg"
rng = random.Random(2718)
parts = ['''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1800 1000">
<defs>
 <linearGradient id="sky" x2="0" y2="1"><stop stop-color="#092435"/><stop offset=".6" stop-color="#315969"/><stop offset="1" stop-color="#74909a"/></linearGradient>
 <linearGradient id="stone" x2="1" y2=".2"><stop stop-color="#143344"/><stop offset=".5" stop-color="#21485a"/><stop offset="1" stop-color="#0d2a3b"/></linearGradient>
 <linearGradient id="water" x2="0" y2="1"><stop stop-color="#9cdae0" stop-opacity=".65"/><stop offset="1" stop-color="#669aad" stop-opacity="0"/></linearGradient>
 <linearGradient id="rock" x2=".8" y2="1"><stop stop-color="#38515a"/><stop offset=".4" stop-color="#122f3d"/><stop offset="1" stop-color="#061b27"/></linearGradient>
 <linearGradient id="light"><stop stop-color="#ccdaa9" stop-opacity="0"/><stop offset=".4" stop-color="#d6e8c5" stop-opacity=".09"/><stop offset="1" stop-color="#eef2c6" stop-opacity="0"/></linearGradient>
 <radialGradient id="halo"><stop stop-color="#d2e5c0" stop-opacity=".35"/><stop offset="1" stop-color="#bcd9c1" stop-opacity="0"/></radialGradient>
 <filter id="soft"><feGaussianBlur stdDeviation="16"/></filter>
 <filter id="glow"><feGaussianBlur stdDeviation="3"/></filter>
 <g id="arch"><path d="M-90 0v-190a90 90 0 0 1 180 0V0H53v-182a53 53 0 0 0-106 0V0Z" fill="url(#stone)"/><path d="M-92-3v-187a92 92 0 0 1 184 0V-3" fill="none" stroke="#56818a" stroke-opacity=".36" stroke-width="4"/></g>
</defs>
<path fill="url(#sky)" d="M0 0h1800v1000H0z"/>
<ellipse cx="1220" cy="240" rx="470" ry="420" fill="url(#halo)"/>
<circle cx="1335" cy="184" r="67" fill="#9cb4b3" opacity=".12"/>
<path d="M330 540 570 155 637 267 715 209 950 589 1090 424 1256 85 1420 410 1510 277 1800 550v450H330Z" fill="#527080" opacity=".5"/>
<path d="m485 462 93-302 38 200 91-110 62 301 143 16 133-81 85-117 70 219 164-271 141 241 142-102 163 29v515H380Z" fill="#315469" opacity=".7"/>
<path d="M880 0 1210 0 788 990H510Z" fill="url(#light)"/>
<path d="m1320 0 126 0-221 1000H995Z" fill="url(#light)"/>
''']

# Far towers and a colossal, broken aqueduct.
for x, y, height, width in [(720,510,210,38),(875,530,310,50),(1010,470,195,32),(1190,540,350,60),(1450,565,245,46),(1535,550,315,45),(1650,575,180,38)]:
    parts.append(f'<path d="M{x-width/2} {y}v{-height}h{width}v{height}Z" fill="#294b5e"/><path d="m{x-width/2-5} {y-height} {width/2+5} -{width*.55} {width/2+5} {width*.55}Z" fill="#365c6b"/>')
    for i in range(3):
        parts.append(f'<path d="M{x-3} {y-height+35+i*45}h6v17h-6Z" fill="#a6cfbd" opacity=".25"/>')
parts.append('''<g opacity=".52" transform="translate(1200 581) scale(1.18)"><use href="#arch" x="-340"/><use href="#arch" x="-160"/><use href="#arch" x="20"/><use href="#arch" x="380"/><path d="M-447-280h630v27h-630zM285-280h190v27H285z" fill="#25495a"/></g>
<path d="M1120 436v368M1143 436v301M1600 508v400" stroke="url(#water)" stroke-width="14"/>
<ellipse cx="1190" cy="688" rx="430" ry="39" fill="#a1c0c4" opacity=".11" filter="url(#soft)"/>
<path d="M660 790 785 648 863 657 916 712 986 683 1058 725 1170 673 1324 740 1497 649 1700 741 1800 701v299H620Z" fill="#1a3b4b"/>
''')
# Monument in the right third; nested empty arch looking into distant world.
parts.append('''<g transform="translate(1390 710)">
<path d="M-170 0v-412c0-260 352-260 352 0V0h-78v-407c0-147-197-147-197 0V0Z" fill="url(#stone)"/>
<path d="M-170-2v-410c0-260 352-260 352 0V0" fill="none" stroke="#57828b" stroke-opacity=".36" stroke-width="8"/>
<path d="M-102-4v-402c0-157 217-157 217 0V0" fill="none" stroke="#0b2637" stroke-width="17"/>
<path d="M-194-16h122v38h-122zM87-16h122v38H87z" fill="#32505a"/>
<path d="M-178-306h96v13h-96zM96-306h96v13H96zM-162-433l40 12-18 27M117-257l32 17-12 53" fill="none" stroke="#092b3a" stroke-width="6"/>
<path d="M-137-42v-286M150-68v-265" stroke="#699ca2" stroke-opacity=".25" stroke-width="3"/>
<path d="M-130-373v43m0 21v15m274-59v43" stroke="#94d9cd" stroke-width="3" opacity=".65"/>
</g>
<path d="m1065 814 65-47 85 12 27 29 104-32 57 4 47 24 100-40 57 33 113 17 80-62v248H960Z" fill="url(#rock)"/>
<path d="m1127 767 99 17 19 22 100-34 60 2 43 26 105-40 66 35 181 19" fill="none" stroke="#687565" stroke-opacity=".55" stroke-width="7"/>
<path d="m1212 815-31 169 74-93 67-96M1480 839l48 120 27-132" fill="none" stroke="#294957" stroke-width="3"/>
<path d="M670 974c89-174 209-236 380-240 125-3 137 58 225 53" fill="none" stroke="#1a3a47" stroke-width="31"/>
<path d="M670 957c89-163 211-222 380-226 125-3 137 58 225 53" fill="none" stroke="#768879" stroke-opacity=".5" stroke-width="7"/>
''')
# Tiny Nori silhouette, scarf and backpack on a foreground ledge.
parts.append('''<g transform="translate(1241 761) rotate(-9)">
<ellipse cx="2" cy="13" rx="22" ry="5" fill="#021922" opacity=".55"/>
<path d="M-5-20q-15-20-10-35 15 3 21 30M9-23q3-30 15-30 6 20-7 33" fill="#6caaa5"/>
<ellipse cx="3" cy="-6" rx="15" ry="21" fill="#ddd5b6"/>
<ellipse cx="9" cy="-24" rx="17" ry="15" fill="#e7dfc3"/>
<path d="M-3-10q-14 0-17 7v16h15z" fill="#716346" stroke="#21343b" stroke-width="2"/>
<path d="M-20 0h14" stroke="#bfa673" stroke-width="2"/>
<ellipse cx="19" cy="-24" rx="3" ry="4" fill="#152c34"/>
<circle cx="20" cy="-25" r="1" fill="#fff9e8"/>
<path d="M-10-13q16 8 28-2l-1 6q-17 6-29-1Z" fill="#be6654"/>
<path d="M-9-10Q-36-6-55-23q0 13-13 14 34 19 61 5Z" fill="#ac5b4d"/>
<ellipse cx="-2" cy="13" rx="8" ry="4" fill="#578482"/><ellipse cx="15" cy="12" rx="8" ry="4" fill="#578482"/>
</g>
<path d="M0 995 159 890 290 929 466 862 637 919 763 854 915 891 1040 952 1140 1000Z" fill="#061c29"/>
''')
# Wind-swept foliage and a controlled scattering of glowing motes.
for i in range(70):
    x = rng.uniform(1030, 1790); y = rng.uniform(802, 950)
    if x < 1220 and y < 865:
        continue
    h = rng.uniform(5, 22)
    parts.append(f'<path d="M{x:.1f} {y:.1f}q{-h*.4:.1f} {-h:.1f} {h*.1:.1f} {-h*1.5:.1f}" fill="none" stroke="{rng.choice(["#36534f","#385e5a","#607a67"])}" stroke-width="1.5"/>')
for i in range(38):
    x, y = rng.uniform(760, 1750), rng.uniform(370, 805)
    parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{rng.uniform(1,2.4):.1f}" fill="#d7d5a0" opacity="{rng.uniform(.15,.65):.2f}"/>')
parts.append('</svg>')
OUT.write_text('\n'.join(parts))
print(OUT)
