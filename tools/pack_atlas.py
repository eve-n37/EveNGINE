#!/usr/bin/env python3
"""Pack original straight-alpha PNG sprites for LotA Canvas rendering."""
import argparse,json,math
from pathlib import Path
from PIL import Image
p=argparse.ArgumentParser();p.add_argument('--sprites',type=Path,required=True);p.add_argument('--font',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
entries=json.loads(a.sprites.read_text());images={}
for name,entry in entries.items():
    if not name or not isinstance(entry,dict):raise ValueError('Expected named sprite entries')
    img=Image.open(a.sprites.parent/entry['file']).convert('RGBA')
    if img.width>2046 or img.height>2046:raise ValueError('Sprite is too large')
    offset=entry.get('offset',[0,0])
    if len(offset)!=2 or not all(isinstance(v,(int,float)) for v in offset):raise ValueError('Offset must be a coordinate pair')
    images[name]=img
width=512;x=0;y=160;row=0;catalog={}
if any(img.width>width for img in images.values()):width=2048
for name,img in sorted(images.items(),key=lambda item:(-item[1].height,item[0])):
    if x+img.width+1>width:x=0;y+=row+1;row=0
    catalog[name]={'source':[x,y,img.width,img.height],'offset':entries[name].get('offset',[0,0])};x+=img.width+1;row=max(row,img.height)
height=max(256,2**math.ceil(math.log2(y+row+1)))
if height>4096:raise ValueError('Atlas exceeds the engine limit')
atlas=Image.new('RGBA',(width,height));font=Image.open(a.font).convert('RGBA')
if font.size!=(192,32):raise ValueError('Font strip must be 192x32')
atlas.paste(font,(0,128))
for name,img in images.items():atlas.paste(img,tuple(catalog[name]['source'][:2]))
a.output.mkdir(parents=True,exist_ok=True)
with (a.output/'atlas.pam').open('wb') as f:
    f.write(f'P7\nWIDTH {width}\nHEIGHT {height}\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n'.encode());f.write(atlas.tobytes())
(a.output/'sprites.json').write_text(json.dumps({'sprites':catalog},indent=2)+'\n')
print(f'{len(catalog)} sprites packed into {width}x{height} straight RGBA atlas')
