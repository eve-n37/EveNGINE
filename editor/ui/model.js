/* Shared editable schema. Pixel strings are two hexadecimal palette indices per pixel. */
(function(root){
'use strict';
const clone=x=>JSON.parse(JSON.stringify(x));
const blank=(w,h,c=0)=>new Array(w*h).fill(c);
const pixelString=a=>a.map(x=>x.toString(16).padStart(2,'0')).join('');
const decode=s=>s.match(/../g).map(x=>parseInt(x,16));
const palette=['#00000000','#497657','#588760','#a5b58b','#b89c6c','#d0b887','#846b4c','#48525e','#687582','#a0aab3','#d7ddd7','#183440','#36778b','#64a9ad','#d8c9a4','#927253','#3b373c','#dca67b','#915b48','#b55e4d','#edd99b','#627ca3','#b2c4d1','#334856','#2d5344','#3c6650'];
function asset(id,name,w,h,fn){let a=blank(w,h);for(let y=0;y<h;y++)for(let x=0;x<w;x++)a[y*w+x]=fn(x,y)||0;return {id,name,width:w,height:h,pixels:pixelString(a),collider:{x:0,y:0,w:0,h:0}};}
function character(id,dir,f){return asset(id,dir+' '+(f+1),16,24,(x,y)=>{
 let step=[0,1,0,-1][f],side=dir==='east'||dir==='west',head=side?(dir==='east'?9:6):8;
 if(y>=3&&y<=8&&Math.abs(x-head)<=3)return y<5?15:17;
 if(y>=9&&y<=19&&x>=4&&x<=11)return x<7?23:21;
 if(y>=11&&y<=16&& (x===3+Math.max(0,step)||x===12-Math.max(0,-step)))return 17;
 if(y>=20&&y<=22&&((x>=4+step&&x<=6+step)||(x>=9-step&&x<=11-step)))return 16;
 return 0;});}
function createProject(name='Mosslight Workshop'){
 let tiles=[asset('grass','Meadow',16,16,(x,y)=>((x*7+y*13)%37===0?2:1)),asset('path','Pebble path',16,16,(x,y)=>(x*11+y*3)%41<2?6:(x+y*7)%17===0?5:4),asset('stone','Stone wall',16,16,(x,y)=>y%8===0||(x+(y>=8?8:0))%16===0?7:8),asset('water','Still water',16,16,(x,y)=>(y%6===0&&x%8<4)?13:12),asset('flowers','Wildflowers',16,16,(x,y)=>(x===4&&y===5)||(x===11&&y===11)?20:(x===4&&y===6)||(x===11&&y===12)?2:0)];
 let sprites=[];for(let d of ['south','north','east','west'])for(let f=0;f<4;f++)sprites.push(character(d+f,d,f));
 sprites.push(asset('crystal','Amber crystal',16,16,(x,y)=>Math.abs(x-8)+Math.abs(y-7)<6?(x<8?20:5):0));
 sprites.push(asset('tree','Young pine',32,48,(x,y)=>y>=35&&y<46&&x>=14&&x<18?15:(y>=5&&y<=38&&Math.abs(x-16)<Math.min(14,(y%12+4)))?(x<16?24:25):0));
 sprites.at(-1).collider={x:13,y:36,w:6,h:10};
 function scene(id,title){let w=32,h=20,base=blank(w,h),detail=blank(w,h,-1),collision=blank(w,h),water=blank(w,h);for(let y=0;y<h;y++)for(let x=0;x<w;x++){let i=y*w+x;if(x===0||y===0||x===w-1||y===h-1){base[i]=2;collision[i]=1;}else if(y===9||y===10)base[i]=1;else if((x*7+y*11)%53===0)detail[i]=4;}return{id,title,width:w,height:h,base,detail,collision,water,spawn:{x:256,y:168},objects:[],portals:[]};}
 let a=scene('meadow','Mosslight Meadow'),b=scene('grove','The Quiet Grove');
 for(let y=3;y<8;y++)for(let x=22;x<28;x++){let i=y*32+x;a.base[i]=3;a.water[i]=1;}
 a.objects=[{id:'pine1',sprite:'tree',kind:'decoration',x:96,y:64,text:''},{id:'prism1',sprite:'crystal',kind:'collectible',x:192,y:140,text:''},{id:'prism2',sprite:'crystal',kind:'collectible',x:360,y:220,text:''}];
 a.portals=[{id:'exit1',x:464,y:144,w:16,h:32,target:'grove',arrival:{x:40,y:168}}];b.portals=[{id:'return1',x:16,y:144,w:16,h:32,target:'meadow',arrival:{x:440,y:168}}];
 b.objects=[{id:'pine2',sprite:'tree',kind:'decoration',x:170,y:70,text:''},{id:'sign',sprite:'crystal',kind:'sign',x:250,y:135,text:'A quiet place to start your next game.'}];
 let project={format:'lota-studio',version:1,id:'workshop-'+Math.random().toString(36).slice(2,10),title:name,palette:clone(palette),tiles,sprites,animations:['south','north','east','west'].map(d=>({id:'walk-'+d,direction:d,state:'walk',frames:[0,1,2,3].map(f=>({sprite:d+f,ms:140}))})),scenes:[a,b],start:'meadow',player:{speed:64,runMultiplier:1.7}};if(root.StudioSampleAudio){project.audio=clone(root.StudioSampleAudio);for(let s of project.scenes)s.music='mosslight-music';}return project;
}
function createBlankProject(name='Untitled adventure'){let p=createProject(name),s=p.scenes[0];s.title='Starting map';s.base=blank(s.width,s.height);s.detail=blank(s.width,s.height,-1);s.collision=blank(s.width,s.height);s.water=blank(s.width,s.height);s.objects=[];s.portals=[];p.scenes=[s];delete p.audio;delete s.music;return p;}
function validate(p,issues=null){let context={view:"setup"},errors=[],bad=(s,where=context)=>{errors.push(s);if(issues)issues.push({message:s,...where});},finite=x=>typeof x==='number'&&Number.isFinite(x),ident=x=>typeof x==='string'&&/^[A-Za-z0-9_-]{1,64}$/.test(x);
 if(!p||p.format!=='lota-studio'||p.version!==1)return['Not a supported EveNGINE Studio project.'];
 if(!ident(p.id))bad('Project ID is invalid.');if(typeof p.title!=='string'||p.title.length<1||p.title.length>80)bad('Project title must be 1–80 characters.');
 if(!Array.isArray(p.palette)||p.palette.length<2||p.palette.length>256||p.palette.some(c=>!/^#[0-9a-fA-F]{6}([0-9a-fA-F]{2})?$/.test(c)))bad('Invalid palette.');
 if(!Array.isArray(p.tiles)||!p.tiles.length||p.tiles.length>64)bad('Use 1–64 tiles.');if(!Array.isArray(p.sprites)||p.sprites.length>256)bad('Use at most 256 sprite frames.');
 if(errors.length)return errors;
 let all=[...p.tiles,...p.sprites],ids=new Set();for(let a of all){context={view:"pixels",asset:a.id,kind:p.tiles.includes(a)?"tile":"sprite"};if(!ident(a.id)||ids.has(a.id))bad('Asset IDs must be unique.');ids.add(a.id);if(!Number.isInteger(a.width)||!Number.isInteger(a.height)||a.width<1||a.height<1||a.width>64||a.height>64)bad('Asset '+a.id+' must be at most 64 × 64.');else if(typeof a.pixels!=='string'||a.pixels.length!==a.width*a.height*2||!/^[0-9a-fA-F]+$/.test(a.pixels)||decode(a.pixels).some(v=>v>=p.palette.length))bad('Invalid pixels in '+a.id);let c=a.collider;if(!c||!['x','y','w','h'].every(k=>finite(c[k]))||c.x<0||c.y<0||c.w<0||c.h<0||c.x+c.w>a.width||c.y+c.h>a.height)bad('Invalid collider in '+a.id);}
 if(p.tiles.some(a=>a.width!==16||a.height!==16))bad('Tiles must be 16 × 16.');
 if(!Array.isArray(p.scenes)||p.scenes.length<1||p.scenes.length>64)return [...errors,'Use 1–64 maps.'];
 let scenes=new Map(p.scenes.map(s=>[s.id,s]));if(scenes.size!==p.scenes.length||!scenes.has(p.start))bad('Duplicate map IDs or missing starting map.');
 let inMap=(s,x,y)=>finite(x)&&finite(y)&&x>=0&&y>=0&&x<s.width*16&&y<s.height*16;
 let rectBlocked=(s,x,y)=>{let l=Math.floor((x-5)/16),r=Math.floor((x+4.999)/16),t=Math.floor((y-7)/16),b=Math.floor((y-.001)/16);for(let yy=t;yy<=b;yy++)for(let xx=l;xx<=r;xx++)if(xx<0||yy<0||xx>=s.width||yy>=s.height||s.collision[yy*s.width+xx])return true;for(let o of s.objects||[]){let a=p.sprites.find(a=>a.id===o.sprite),c=a?.collider;if(c&&c.w>0&&c.h>0&&x-5<o.x+c.x+c.w&&x+5>o.x+c.x&&y-7<o.y+c.y+c.h&&y>o.y+c.y)return true;}return false;};
 for(let s of p.scenes){context={view:"map",scene:s.id};if(!ident(s.id)||typeof s.title!=='string')bad('Map ID or title invalid.');if(!Number.isInteger(s.width)||!Number.isInteger(s.height)||s.width<1||s.height<1||s.width>256||s.height>256){bad('Invalid map dimensions.');continue;}for(let k of ['base','detail','collision','water'])if(!Array.isArray(s[k])||s[k].length!==s.width*s.height||s[k].some(v=>!Number.isInteger(v)||(k==='base'?(v<0||v>=p.tiles.length):k==='detail'?(v< -1||v>=p.tiles.length):(v<0||v>1))))bad('Invalid '+k+' layer in '+s.title);
 if(!s.spawn||!inMap(s,s.spawn.x,s.spawn.y)||rectBlocked(s,s.spawn.x,s.spawn.y))bad('Spawn is outside the map or blocked in '+s.title);
 if(!Array.isArray(s.objects)||s.objects.length>4096||!Array.isArray(s.portals)||s.portals.length>512){bad('Invalid objects or exits.');continue;}
 let objectIds=new Set();for(let o of s.objects){context={view:"map",scene:s.id,object:o.id};if(!ident(o.id)||objectIds.has(o.id))bad('Duplicate/invalid object ID.');objectIds.add(o.id);if(!p.sprites.some(a=>a.id===o.sprite)||!['decoration','collectible','sign','npc','chest','door','switch','enemy'].includes(o.kind)||!inMap(s,o.x,o.y)||typeof o.text!=='string'||o.text.length>180)bad('Invalid object in '+s.title);}
 for(let e of s.portals){context={view:"map",scene:s.id,exit:e.id};let target=scenes.get(e.target);if(!ident(e.id)||!target||!['x','y','w','h'].every(k=>finite(e[k]))||e.w<=0||e.h<=0||e.x<0||e.y<0||e.x+e.w>s.width*16||e.y+e.h>s.height*16||!e.arrival||!inMap(target,e.arrival.x,e.arrival.y)||rectBlocked(target,e.arrival.x,e.arrival.y))bad('Invalid or blocked exit in '+s.title);}}
 context={view:'animation'};if(!Array.isArray(p.animations)||p.animations.length>64)bad('Invalid animation list.');else {let keys=new Set();for(let a of p.animations){let key=a.state+'-'+a.direction;if(keys.has(key)||!ident(a.id)||!['south','north','east','west'].includes(a.direction)||!['walk','idle','attack'].includes(a.state)||!Array.isArray(a.frames)||a.frames.length<1||a.frames.length>64||a.frames.some(f=>!p.sprites.some(s=>s.id===f.sprite)||!Number.isInteger(f.ms)||f.ms<16||f.ms>5000))bad('Invalid animation '+a.id);keys.add(key);}for(let d of ['south','north','east','west'])if(!keys.has('walk-'+d))bad('Missing '+d+' walking animation.');}
 context={view:"assets"};validateExtras(p,bad);validateInteractions(p,bad);validateStory(p,bad);validateAppearance(p,bad);(root.EveAdventureModel||(typeof require!=='undefined'?require('./adventure-model.js'):null))?.validate(p,bad);(root.EveRCModel||(typeof require!=='undefined'?require('./rc-model.js'):null))?.validate(p,bad);
 context={view:"setup"};if(!p.player||!finite(p.player.speed)||p.player.speed<1||p.player.speed>512||!finite(p.player.runMultiplier)||p.player.runMultiplier<1||p.player.runMultiplier>4)bad('Invalid movement speed.');return errors;
}
class History{constructor(p){this.project=clone(p);this.undoStack=[];this.redoStack=[];}commit(before){if(JSON.stringify(before)===JSON.stringify(this.project))return false;this.undoStack.push(before);if(this.undoStack.length>60)this.undoStack.shift();this.redoStack=[];this.trim();return true;}trim(){let bytes=0;for(let i=this.undoStack.length-1;i>=0;i--){bytes+=JSON.stringify(this.undoStack[i]).length;if(bytes>64*1024*1024){this.undoStack.splice(0,i+1);break;}}}undo(){if(!this.undoStack.length)return false;this.redoStack.push(clone(this.project));this.project=this.undoStack.pop();return true;}redo(){if(!this.redoStack.length)return false;this.undoStack.push(clone(this.project));this.project=this.redoStack.pop();return true;}}
function fill(a,w,h,x,y,v){let old=a[y*w+x];if(old===v)return;let q=[[x,y]];a[y*w+x]=v;while(q.length){let [xx,yy]=q.pop();for(let [nx,ny] of [[xx-1,yy],[xx+1,yy],[xx,yy-1],[xx,yy+1]])if(nx>=0&&ny>=0&&nx<w&&ny<h&&a[ny*w+nx]===old){a[ny*w+nx]=v;q.push([nx,ny]);}}}
function transform(a,w,h,mode){if(mode==='rotate'&&w!==h)throw Error('Rotation requires a square asset.');let out=blank(w,h);for(let y=0;y<h;y++)for(let x=0;x<w;x++){let nx=mode==='flipX'?w-1-x:mode==='rotate'?h-1-y:x,ny=mode==='flipY'?h-1-y:mode==='rotate'?x:y;out[ny*w+nx]=a[y*w+x];}return out;}

// Optional v0.5 fields preserve v1 projects. Imported assets and sounds stay inline.
function base64Bytes(s){
 if(typeof s!=='string'||s.length>5592410||s.length%4||!/^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$/.test(s))throw Error('Invalid or oversized WAV data.');
 if(typeof Buffer!=='undefined')return new Uint8Array(Buffer.from(s,'base64'));
 return Uint8Array.from(atob(s),c=>c.charCodeAt(0));
}
function wavInfo(bytes){
 if(bytes.length<44||bytes.length>4*1024*1024)throw Error('WAV must be at most 4 MiB.');
 let v=new DataView(bytes.buffer,bytes.byteOffset,bytes.byteLength),tag=i=>String.fromCharCode(...bytes.slice(i,i+4));
 if(tag(0)!=='RIFF'||tag(8)!=='WAVE'||v.getUint32(4,true)+8!==bytes.length)throw Error('Invalid RIFF/WAVE file.');
 let fmt=null,data=null,at=12;
 while(at+8<=bytes.length){let n=v.getUint32(at+4,true),end=at+8+n;if(end>bytes.length)throw Error('Truncated WAV chunk.');
  if(tag(at)==='fmt '){if(fmt||n<16)throw Error('Invalid WAV format chunk.');let x=at+8;fmt={format:v.getUint16(x,true),channels:v.getUint16(x+2,true),rate:v.getUint32(x+4,true),byteRate:v.getUint32(x+8,true),align:v.getUint16(x+12,true),bits:v.getUint16(x+14,true)};}
  if(tag(at)==='data'){if(data)throw Error('Duplicate WAV data chunk.');data={offset:at+8,length:n};}at=end+(n%2);
 }
 if(at!==bytes.length||!fmt||!data||fmt.format!==1||fmt.bits!==16||![1,2].includes(fmt.channels)||fmt.rate<8000||fmt.rate>48000||fmt.align!==fmt.channels*2||fmt.byteRate!==fmt.rate*fmt.align||data.length===0||data.length%fmt.align)throw Error('Use PCM 16-bit WAV, mono or stereo, 8–48 kHz.');
 let seconds=data.length/fmt.byteRate;if(seconds>60)throw Error('WAV clips must be 60 seconds or shorter.');return {...fmt,...data,seconds};
}
function importSheet(p,rgba,width,height,o){
 let w=o.width,h=o.height,margin=o.margin||0,gap=o.gap||0,kind=o.kind;
 if(!['tile','sprite'].includes(kind)||![w,h,margin,gap,width,height].every(Number.isInteger)||w<1||h<1||w>64||h>64||margin<0||gap<0||(kind==='tile'&&(w!==16||h!==16))||rgba.length!==width*height*4)throw Error('Invalid sheet grid.');
 let cols=Math.floor((width-2*margin+gap)/(w+gap)),rows=Math.floor((height-2*margin+gap)/(h+gap));
 if(cols<1||rows<1||2*margin+cols*w+(cols-1)*gap!==width||2*margin+rows*h+(rows-1)*gap!==height)throw Error('The grid must exactly fit the sheet, including margin and spacing.');
 let list=kind==='tile'?p.tiles:p.sprites,count=cols*rows;if(count+list.length>(kind==='tile'?64:256))throw Error('Sheet exceeds the '+(kind==='tile'?64:256)+' '+kind+' limit.');
 let pal=clone(p.palette),assets=[],prefix='sheet-'+Date.now().toString(36)+'-'+Math.random().toString(36).slice(2,6),normal=c=>c.length===7?c.toLowerCase()+'ff':c.toLowerCase();
 for(let row=0;row<rows;row++)for(let col=0;col<cols;col++){let pix=[];for(let y=0;y<h;y++)for(let x=0;x<w;x++){let i=((margin+row*(h+gap)+y)*width+margin+col*(w+gap)+x)*4,n=0;if(rgba[i+3]){let c='#'+Array.from(rgba.slice(i,i+4)).map(v=>v.toString(16).padStart(2,'0')).join('');n=pal.findIndex(v=>normal(v)===c);if(n<0){if(pal.length===256)throw Error('Sheet exceeds the 256-colour palette.');n=pal.length;pal.push(c);}}pix.push(n);}
  let index=assets.length;assets.push({id:prefix+'-'+index,name:(o.name||'Imported')+' '+(index+1),group:o.group||o.name||'Imported',width:w,height:h,pixels:pixelString(pix),collider:{x:0,y:0,w:0,h:0}});
 }
 return {assets,palette:pal,columns:cols,rows};
}
function assetUsage(p,kind,a){let n=0;if(kind==='tile'){let i=p.tiles.indexOf(a);for(let r of p.autotiles||[])n+=r.tiles.filter(id=>id===a.id).length;for(let s of p.scenes)for(let k of ['base','detail'])n+=s[k].filter(v=>v===i).length;for(let st of p.stamps||[])for(let k of ['base','detail'])n+=(st.layers[k]||[]).filter(v=>v===a.id).length;}else{for(let s of p.scenes)n+=s.objects.filter(o=>o.sprite===a.id||o.openSprite===a.id||o.portrait===a.id).length;if(p.menus?.artwork===a.id)n++;for(let item of p.items||[])if(item.icon===a.id)n++;for(let c of p.animations)n+=c.frames.filter(f=>f.sprite===a.id).length;}return n;}
function deleteAsset(p,kind,id){let list=kind==='tile'?p.tiles:p.sprites,i=list.findIndex(a=>a.id===id);if(i<0)throw Error('Unknown asset.');if(assetUsage(p,kind,list[i]))throw Error('This asset is still used. Replace its map, stamp or animation references first.');if(kind==='tile'&&list.length===1)throw Error('Keep at least one tile.');list.splice(i,1);if(kind==='tile')for(let s of p.scenes)for(let k of ['base','detail'])s[k]=s[k].map(v=>v>i?v-1:v);}
function copyRegion(p,s,r,keys=['base','detail','collision','water']){
 if(!['x','y','w','h'].every(k=>Number.isInteger(r[k]))||r.w<1||r.h<1||r.w>64||r.h>64||r.x<0||r.y<0||r.x+r.w>s.width||r.y+r.h>s.height)throw Error('Select an area within the map, up to 64 × 64 tiles.');
 let st={id:'stamp-'+Date.now().toString(36),name:'Map stamp',width:r.w,height:r.h,layers:{}};for(let k of keys){if(!['base','detail','collision','water'].includes(k))throw Error('Unknown layer.');let a=[];for(let y=0;y<r.h;y++)for(let x=0;x<r.w;x++){let v=s[k][(y+r.y)*s.width+x+r.x];a.push(k==='base'||k==='detail'?(v<0?null:p.tiles[v].id):v);}st.layers[k]=a;}return st;
}
function validateStamp(p,st){if(!st||typeof st.id!=='string'||!/^[A-Za-z0-9_-]{1,64}$/.test(st.id)||typeof st.name!=='string'||st.name.length<1||st.name.length>80||!Number.isInteger(st.width)||!Number.isInteger(st.height)||st.width<1||st.height<1||st.width>64||st.height>64||!st.layers||!Object.keys(st.layers).length)throw Error('Invalid map stamp.');for(let [k,a] of Object.entries(st.layers)){if(!['base','detail','collision','water'].includes(k)||!Array.isArray(a)||a.length!==st.width*st.height||a.some(v=>k==='base'||k==='detail'?!((k==='detail'&&v===null)||p.tiles.some(t=>t.id===v)):v!==0&&v!==1))throw Error('Invalid stamp layer or missing tile.');}}
function pasteRegion(p,s,st,x,y){validateStamp(p,st);if(!Number.isInteger(x)||!Number.isInteger(y))throw Error('Invalid stamp position.');let clipped=0,ids=new Map(p.tiles.map((a,i)=>[a.id,i]));for(let yy=0;yy<st.height;yy++)for(let xx=0;xx<st.width;xx++){if(x+xx<0||y+yy<0||x+xx>=s.width||y+yy>=s.height){clipped++;continue;}for(let [k,a] of Object.entries(st.layers)){let v=a[yy*st.width+xx];let index=(y+yy)*s.width+x+xx;s[k][index]=k==='base'||k==='detail'?(v===null?-1:ids.get(v)):v;if(k==='base'&&s.autotileCells)s.autotileCells[index]='';}}return clipped;}
function clearRegion(s,r,keys){for(let k of keys)for(let y=r.y;y<r.y+r.h;y++)for(let x=r.x;x<r.x+r.w;x++){s[k][y*s.width+x]=k==='detail'?-1:0;if(k==='base'&&s.autotileCells)s.autotileCells[y*s.width+x]='';}}
function transformStamp(st,mode){let out=clone(st);if(mode==='rotate'){out.width=st.height;out.height=st.width;}for(let [k,a] of Object.entries(st.layers)){let next=[];for(let y=0;y<st.height;y++)for(let x=0;x<st.width;x++){let nx=mode==='flipX'?st.width-1-x:mode==='rotate'?st.height-1-y:x,ny=mode==='flipY'?st.height-1-y:mode==='rotate'?x:y;next[ny*out.width+nx]=a[y*st.width+x];}out.layers[k]=next;}return out;}
function validateExtras(p,bad){
 for(let a of [...p.tiles,...p.sprites]){if(a.group!==undefined&&(typeof a.group!=='string'||a.group.length>40))bad('Asset groups must be at most 40 characters.');if(['solid','water'].some(k=>a[k]!==undefined&&typeof a[k]!=='boolean'))bad('Tile flags must be booleans.');}
 if(p.stamps!==undefined){if(!Array.isArray(p.stamps)||p.stamps.length>64)bad('Use at most 64 stamps.');else{let ids=new Set();for(let st of p.stamps){try{validateStamp(p,st);if(ids.has(st.id))throw Error('Duplicate stamp ID.');ids.add(st.id);}catch(e){bad(e.message);}}}}
 let ids=new Set();if(p.audio!==undefined){let a=p.audio;if(!a||!Array.isArray(a.assets)||a.assets.length>32||!Number.isFinite(a.musicVolume)||a.musicVolume<0||a.musicVolume>1||!Number.isFinite(a.sfxVolume)||a.sfxVolume<0||a.sfxVolume>1||!a.cues)bad('Invalid audio settings.');else{let total=0;for(let clip of a.assets){try{if(!clip||typeof clip.id!=='string'||!/^[A-Za-z0-9_-]{1,64}$/.test(clip.id)||ids.has(clip.id)||typeof clip.name!=='string'||clip.name.length<1||clip.name.length>80)throw Error('Invalid audio asset.');let bytes=base64Bytes(clip.data);wavInfo(bytes);total+=bytes.length;ids.add(clip.id);}catch(e){bad(e.message);}}if(total>12*1024*1024)bad('Project audio exceeds 12 MiB.');for(let [key,value] of Object.entries(a.cues))if(!['collect','attack','exit','interact','chest','door','switch'].includes(key)||typeof value!=='string'||(value&&!ids.has(value)))bad('Unknown audio cue or clip.');}}
 for(let s of p.scenes)if(s.music!==undefined&&(typeof s.music!=='string'||(s.music&&!ids.has(s.music))))bad('Missing map music.');
 if(new TextEncoder().encode(JSON.stringify(p)).length>32*1024*1024)bad('Project exceeds the 32 MiB runtime limit.');
}


function validateInteractions(p,report){const bad=message=>report(message,{view:"items"});
 const ident=v=>typeof v==='string'&&/^[A-Za-z0-9_-]{1,64}$/.test(v),items=p.items||[],itemIds=new Set();
 if(!Array.isArray(items)||items.length>64){bad('Use at most 64 item definitions.');return;}
 for(let item of items){if(!item||!ident(item.id)||itemIds.has(item.id)||typeof item.name!=='string'||item.name.length<1||item.name.length>40||!p.sprites.some(a=>a.id===item.icon))bad('Invalid inventory item or icon.');else itemIds.add(item.id);}
 for(let s of p.scenes)for(let o of s.objects){const bad=message=>report(message,{view:"map",scene:s.id,object:o.id});
  if(o.openSprite!==undefined&&(typeof o.openSprite!=='string'||o.openSprite&&!p.sprites.some(a=>a.id===o.openSprite)))bad('Missing alternate sprite in '+o.id);
  for(let key of ['item','requiredItem'])if(o[key]!==undefined&&(typeof o[key]!=='string'||o[key]&&!itemIds.has(o[key])))bad('Unknown item in '+o.id);
  if(o.quantity!==undefined&&(!Number.isInteger(o.quantity)||o.quantity<1||o.quantity>99))bad('Item quantities must be 1–99.');
  for(let key of ['consumeKey','switchOnly'])if(o[key]!==undefined&&typeof o[key]!=='boolean')bad('Invalid interaction toggle.');
  if(o.kind==='chest'&&!itemIds.has(o.item))bad('Choose a reward item for chest '+o.id);
  if(o.kind==='door'&&o.switchOnly&&o.requiredItem)bad('Switch-only doors cannot also require an item.');
  if(o.kind==='npc'&&!o.dialogue&&(!Array.isArray(o.pages)||o.pages.length<1||o.pages.length>16||o.pages.some(v=>typeof v!=='string'||v.length<1||v.length>180)||typeof o.speaker!=='string'||o.speaker.length>40))bad('NPC dialogue requires 1–16 pages, at most 180 characters each.');
  if(o.kind==='switch'&&(!Array.isArray(o.targets)||o.targets.length<1||o.targets.length>32||new Set(o.targets).size!==o.targets.length||o.targets.some(id=>!s.objects.some(target=>target.id===id&&target.kind==='door'))))bad('Switch '+o.id+' must target 1–32 doors on the same map.');
 }
}
function importBuiltins(p,bundle,ids=[],stampIds=[],useCharacter=false){
 const next=clone(p),needed=new Set(ids),requestedStamps=bundle.stamps.filter(st=>stampIds.includes(st.id));
 if(stampIds.some(id=>!bundle.stamps.some(st=>st.id===id)))throw Error('Unknown built-in stamp.');
 for(let st of requestedStamps)for(let k of ['base','detail'])for(let id of st.layers[k]||[])if(id)needed.add(id);
 if(useCharacter)for(let d of ['south','north','east','west'])for(let f=0;f<4;f++)needed.add('eve-hero-'+d+f);
 for(let id of needed){let a=bundle.assets.find(a=>a.id===id);if(!a)throw Error('Unknown built-in artwork.');if(a.preset?.openSprite)needed.add(a.preset.openSprite);}
 const normal=c=>c.length===7?c.toLowerCase()+'ff':c.toLowerCase();let pal=next.palette,added=0;
 for(let id of needed){let a=bundle.assets.find(a=>a.id===id),list=a.kind==='tile'?next.tiles:next.sprites,existing=[...next.tiles,...next.sprites].find(x=>x.id===id);
  if(existing){if(!list.includes(existing)||existing.width!==a.width||existing.height!==a.height)throw Error('A conflicting asset ID already exists: '+id);continue;}
  if(list.length>=(a.kind==='tile'?64:256))throw Error('Built-in import exceeds the '+a.kind+' limit.');
  let copy=clone(a);delete copy.pack;delete copy.kind;delete copy.preset;let remap=new Map([[0,0]]);let pixels=decode(copy.pixels).map(v=>{if(remap.has(v))return remap.get(v);let c=normal(bundle.palette[v]),i=pal.findIndex(x=>normal(x)===c);if(i<0){if(pal.length>=256)throw Error('Built-in import exceeds the palette limit.');i=pal.length;pal.push(bundle.palette[v]);}remap.set(v,i);return i;});copy.pixels=pixelString(pixels);list.push(copy);added++;
 }
 if(useCharacter){for(let d of ['south','north','east','west']){let frames=[0,1,2,3].map(f=>({sprite:'eve-hero-'+d+f,ms:140})),walk=next.animations.find(a=>a.state==='walk'&&a.direction===d);walk.frames=frames;}}
 for(let st of requestedStamps)if(!(next.stamps||[]).some(x=>x.id===st.id)){if((next.stamps||[]).length>=64)throw Error('Stamp limit reached.');(next.stamps||(next.stamps=[])).push(clone(st));}
 let previous=validate(p),errors=validate(next).filter(e=>!previous.includes(e));if(errors.length)throw Error(errors[0]);return {project:next,added};
}
function objectDefaults(p,sprite,bundle,scene){let a=bundle?.assets.find(a=>a.id===sprite),preset=a?.preset?clone(a.preset):{};
 if(preset.openSprite&&!p.sprites.some(a=>a.id===preset.openSprite))delete preset.openSprite;
 if(preset.kind==='chest'){let item=(p.items||[])[0];if(item){preset.item=item.id;preset.quantity=1;}else preset.kind='decoration';}
 if(preset.kind==='switch'){let door=scene?.objects.find(o=>o.kind==='door');if(door)preset.targets=[door.id];else preset.kind='decoration';}
 return {kind:'decoration',text:'',...preset};
}
function paintTerrain(p,s,k,x,y,mode,value,auto){let indices=[];if(mode==='fill'){let old=s[k][y*s.width+x],q=[y*s.width+x],seen=new Set(q);while(q.length){let i=q.pop();indices.push(i);let xx=i%s.width,yy=Math.floor(i/s.width);for(let [nx,ny] of [[xx-1,yy],[xx+1,yy],[xx,yy-1],[xx,yy+1]])if(nx>=0&&ny>=0&&nx<s.width&&ny<s.height){let n=ny*s.width+nx;if(!seen.has(n)&&s[k][n]===old){seen.add(n);q.push(n);}}}}else indices=[y*s.width+x];
 for(let i of indices){s[k][i]=value;if(k==='base'&&s.autotileCells)s.autotileCells[i]='';if(auto&&k==='base'){let a=p.tiles[value];s.collision[i]=a.solid?1:0;s.water[i]=a.water?1:0;}}if(k==='base'&&s.autotileCells)(root.EveRCModel||(typeof require!=='undefined'?require('./rc-model.js'):null))?.updateAutotiles(p,s,indices);
}


function validateStory(p,report){const bad=report;
 const ident=v=>typeof v==='string'&&/^[A-Za-z0-9_-]{1,64}$/.test(v),text=(v,n,empty=false)=>typeof v==='string'&&v.length<=n&&(empty||v.length>0),quantity=v=>v===undefined||Number.isInteger(v)&&v>=1&&v<=99;
 const objects=new Map(p.scenes.flatMap(s=>(s.objects||[]).map(o=>[s.id+':'+o.id,o]))),items=new Set((Array.isArray(p.items)?p.items:[]).map(i=>i.id)),quests=p.quests===undefined?[]:p.quests,ids=new Set();
 if(!Array.isArray(quests)||quests.length>64){bad('Use at most 64 quests.');return;}
 for(let q of quests){const bad=(message,where)=>report(message,where||{view:"quests",quest:q?.id});
  if(!q||!ident(q.id)||ids.has(q.id)){bad('Quest IDs must be unique.');continue;}ids.add(q.id);
  if(!text(q.name,40)||!text(q.description,180,true))bad('Quest names allow 40 characters; descriptions allow 180.');
  if(!Array.isArray(q.objectives)||q.objectives.length<1||q.objectives.length>8){bad('Each quest needs 1–8 objectives.');continue;}
  let seen=new Set();for(let o of q.objectives){if(!o||!text(o.target,129)||!quantity(o.quantity)){bad('Invalid quest objective.');continue;}let key=o.type+':'+o.target;if(seen.has(key))bad('Duplicate quest objective.');seen.add(key);let target=objects.get(o.target);
   if(o.type==='item'){if(!items.has(o.target))bad('Unknown quest item: '+o.target);}
   else if(o.type==='visit'){if(!p.scenes.some(s=>s.id===o.target)||(o.quantity||1)!==1)bad('Unknown quest map or invalid visit quantity.');}
   else if(o.type==='talk'){if(target?.kind!=='npc'||(o.quantity||1)!==1)bad('Talk objectives must reference an NPC.');}
   else if(o.type==='activate'){if(!['chest','door','switch'].includes(target?.kind)||(o.quantity||1)!==1)bad('Activation objectives must reference a chest, door or switch.');}
   else bad('Unknown quest objective type.');
  }
  if(q.rewardItem!==undefined&&(typeof q.rewardItem!=='string'||q.rewardItem&&!items.has(q.rewardItem)))bad('Unknown quest reward item.');if(!quantity(q.rewardQuantity))bad('Quest reward quantities must be 1–99.');
 }
 for(let s of p.scenes)for(let o of s.objects||[])if(o.dialogue!==undefined){const bad=(message,where)=>report(message,where||{view:"dialogue",scene:s.id,object:o.id});let d=o.dialogue;
  if(o.kind!=='npc'||!d||!ident(d.start)||!Array.isArray(d.nodes)||d.nodes.length<1||d.nodes.length>64){bad('Dialogue trees need an NPC, a start node and 1–64 nodes.');continue;}
  if(!text(o.speaker,40,true))bad('Dialogue speaker allows 40 characters.');let nodes=new Map();
  for(let node of d.nodes){const bad=message=>report(message,{view:"dialogue",scene:s.id,object:o.id,node:node?.id});if(!node||!ident(node.id)||nodes.has(node.id)){bad('Dialogue node IDs must be unique.');continue;}nodes.set(node.id,node);if(!text(node.text,180))bad('Dialogue text must be 1–180 characters.');if(!Array.isArray(node.choices)||node.choices.length>3){bad('Use at most three choices per dialogue node.');continue;}
   for(let c of node.choices){if(!c||!text(c.text,40)||typeof c.next!=='string'||c.next&&!ident(c.next)){bad('Invalid dialogue choice or destination.');continue;}
    if(['action','quest','conditionQuest','conditionState','requiredItem'].some(k=>c[k]!==undefined&&typeof c[k]!=='string'))bad('Dialogue action and condition fields must be text.');
    if(c.action? !['accept','complete'].includes(c.action)||!ids.has(c.quest):!!c.quest)bad('Choose an existing quest for a dialogue action.');
    if(c.conditionQuest? !ids.has(c.conditionQuest)||!['available','active','ready','completed'].includes(c.conditionState):!!c.conditionState)bad('Invalid dialogue quest condition.');
    if(c.requiredItem!==undefined&&(typeof c.requiredItem!=='string'||c.requiredItem&&!items.has(c.requiredItem)))bad('Unknown choice requirement item.');if(!quantity(c.quantity))bad('Choice quantities must be 1–99.');
   }
  }
  if(!nodes.has(d.start))bad('Missing starting dialogue node.');for(let node of nodes.values())for(let c of (Array.isArray(node.choices)?node.choices:[]))if(c?.next&&!nodes.has(c.next))bad('Missing dialogue destination: '+c.next,{view:'dialogue',scene:s.id,object:o.id,node:node.id});
  let reached=new Set(),pending=[d.start];while(pending.length){let id=pending.pop();if(reached.has(id)||!nodes.has(id))continue;reached.add(id);for(let c of (Array.isArray(nodes.get(id).choices)?nodes.get(id).choices:[]))if(c?.next)pending.push(c.next);}if(reached.size!==nodes.size)bad('Every dialogue node must be reachable from the start.',{view:'dialogue',scene:s.id,object:o.id,node:[...nodes.keys()].find(k=>!reached.has(k))});
 }
}
function storyItemUsed(p,id){return (root.EveAdventureModel||(typeof require!=='undefined'?require('./adventure-model.js'):null))?.itemUsed(p,id)||(p.events||[]).some(e=>[...e.actions,...e.conditions||[]].some(a=>a.target===id))||(p.quests||[]).some(q=>q.rewardItem===id||q.objectives.some(o=>o.type==='item'&&o.target===id))||p.scenes.some(s=>s.objects.some(o=>o.dialogue?.nodes.some(n=>n.choices.some(c=>c.requiredItem===id))));}
function storyObjectUsed(p,key){return (p.events||[]).some(e=>e.scene+':'+e.object===key||e.actions.some(a=>a.target===key))||(p.quests||[]).some(q=>q.objectives.some(o=>['talk','activate'].includes(o.type)&&o.target===key));}
function renameStoryObject(p,scene,oldId,newId){for(let e of p.events||[]){if(e.scene===scene&&e.object===oldId)e.object=newId;for(let a of e.actions)if(a.target===scene+':'+oldId)a.target=scene+':'+newId;}for(let q of p.quests||[])for(let o of q.objectives)if(['talk','activate'].includes(o.type)&&o.target===scene+':'+oldId)o.target=scene+':'+newId;}


const defaultUITheme={panel:'#121c22',text:'#dbe2d3',accent:'#e9c88a',muted:'#99b5ab',selection:'#304131',panelOpacity:230/255};
function validateAppearance(p,bad){
 if(p.uiTheme!==undefined){let t=p.uiTheme;if(!t||typeof t!=='object'||Array.isArray(t))bad('Invalid game UI theme.');else for(let [key,value] of Object.entries(t)){if(key==='panelOpacity'){if(!Number.isFinite(value)||value<0||value>1)bad('UI panel opacity must be 0–1.');}else if(!['panel','text','accent','muted','selection'].includes(key)||typeof value!=='string'||!/^#[0-9a-fA-F]{6}$/.test(value))bad('UI colors must use #RRGGBB.');}}
 if(p.appIcon!==undefined){let icon=p.appIcon;if(!icon||!Number.isInteger(icon.size)||icon.size<16||icon.size>512||typeof icon.pixels!=='string'||icon.pixels.length!==icon.size*icon.size*8||!/^[0-9a-fA-F]+$/.test(icon.pixels))bad('Game icons require square RGBA artwork, 16–512 pixels.');}
}
function createIcon(size=64){if(!Number.isInteger(size)||size<16||size>512)throw Error('Use an icon size from 16 to 512.');return {size,pixels:'00000000'.repeat(size*size)};}
function iconFromAsset(p,asset,size=64){let icon=createIcon(size),pixels=Array(size*size).fill('00000000'),scale=Math.max(1,Math.floor((size-8)/Math.max(asset.width,asset.height))),w=asset.width*scale,h=asset.height*scale,ox=Math.floor((size-w)/2),oy=Math.floor((size-h)/2),source=decode(asset.pixels);
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){let xx=x+ox,yy=y+oy;if(xx<0||yy<0||xx>=size||yy>=size)continue;let index=source[Math.floor(y/scale)*asset.width+Math.floor(x/scale)],color=index===0?'00000000':p.palette[index].slice(1);pixels[yy*size+xx]=color.length===6?color+'ff':color;}icon.pixels=pixels.join('');return icon;
}
function paintIcon(icon,x,y,color,tool='pencil'){if(!Number.isInteger(x)||!Number.isInteger(y)||x<0||y<0||x>=icon.size||y>=icon.size)return false;let value=tool==='erase'?'00000000':color.replace(/^#/,'').toLowerCase();if(!/^[0-9a-f]{6}([0-9a-f]{2})?$/.test(value))throw Error('Invalid icon paint color.');if(value.length===6)value+='ff';let index=y*icon.size+x,old=icon.pixels.slice(index*8,index*8+8);if(old===value)return false;
 if(tool==='fill'){let pixels=icon.pixels.match(/.{8}/g),q=[index];pixels[index]=value;while(q.length){let i=q.pop(),xx=i%icon.size,yy=Math.floor(i/icon.size);for(let [nx,ny] of [[xx-1,yy],[xx+1,yy],[xx,yy-1],[xx,yy+1]])if(nx>=0&&ny>=0&&nx<icon.size&&ny<icon.size){let at=ny*icon.size+nx;if(pixels[at]===old){pixels[at]=value;q.push(at);}}}icon.pixels=pixels.join('');}
 else icon.pixels=icon.pixels.slice(0,index*8)+value+icon.pixels.slice(index*8+8);return true;
}

function validateDetailed(p){let issues=[];let errors=validate(p,issues);if(!issues.length&&errors.length)issues=errors.map(message=>({message,view:"setup"}));return issues;}
const api={validateDetailed,defaultUITheme,validateAppearance,createIcon,iconFromAsset,paintIcon,validateStory,storyItemUsed,storyObjectUsed,renameStoryObject,clone,blank,pixelString,decode,createProject,createBlankProject,validate,History,fill,transform,wavInfo,base64Bytes,importSheet,assetUsage,deleteAsset,copyRegion,pasteRegion,clearRegion,transformStamp,validateStamp,validateInteractions,importBuiltins,objectDefaults,paintTerrain};if(typeof module!=='undefined')module.exports=api;root.LotaModel=api;
})(typeof window!=='undefined'?window:globalThis);
