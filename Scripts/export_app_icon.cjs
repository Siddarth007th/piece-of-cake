const fs=require('fs');
const sharp=require(process.env.SHARP_PATH || 'sharp');
const path=require('path');
const root=path.resolve(__dirname,'../Build/Mac/Resources');
(async()=>{
 const background=Buffer.from('<svg width="1024" height="1024"><defs><radialGradient id="bg" cx="65%" cy="30%" r="90%"><stop stop-color="#348f89"/><stop offset="1" stop-color="#102839"/></radialGradient></defs><rect x="24" y="24" width="976" height="976" rx="218" fill="url(#bg)"/><rect x="35" y="35" width="954" height="954" rx="209" fill="none" stroke="#9be8c4" stroke-opacity=".35" stroke-width="5"/></svg>');
 const mask=Buffer.from('<svg width="1024" height="1024"><rect x="24" y="24" width="976" height="976" rx="218" fill="white"/></svg>');
 const portrait=await sharp(root+'/NoriPortrait.png').resize(940,940).toBuffer();
 const combined=await sharp(background).composite([{input:portrait,top:60,left:42}]).png().toBuffer();
 await sharp(combined).composite([{input:mask,blend:'dest-in'}]).png().toFile(root+'/NoriIcon.png');
 const out='/private/tmp/PieceOfCake-Nori.iconset';fs.mkdirSync(out,{recursive:true});
 for(const n of [16,32,128,256,512])for(const scale of [1,2])await sharp(root+'/NoriIcon.png').resize(n*scale,n*scale).png().toFile(`${out}/icon_${n}x${n}${scale===2?'@2x':''}.png`);
})();
