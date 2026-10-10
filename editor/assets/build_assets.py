"""Rebuild the original EveNGINE pixel-art library, PNGs and runtime-neutral data."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
import json,math
root=Path(__file__).resolve().parent;ui=root.parent/'ui';png=root/'png';png.mkdir(exist_ok=True)
palette=['#00000000','#497657','#588760','#a5b58b','#b89c6c','#d0b887','#846b4c','#48525e','#687582','#a0aab3','#d7ddd7','#183440','#36778b','#64a9ad','#d8c9a4','#927253','#3b373c','#dca67b','#915b48','#b55e4d','#edd99b','#627ca3','#b2c4d1','#334856','#2d5344','#3c6650','#22302f','#315443','#72975c','#bdcf91','#d8be7b','#8b533d','#673b35','#b77350','#e5b685','#d5a97a','#53627d','#8896ac','#2d4356','#a480b3','#66496f','#ebc9e0','#b8779c','#cf8070','#faf0c1','#f7ce79','#cb9d52','#568b83']
C={i:tuple(bytes.fromhex(c[1:]+('ff' if len(c)==7 else ''))) for i,c in enumerate(palette)}
assets=[];packs=[{'id':'outdoors','name':'Meadow & water','description':'24 terrain tiles with shores, paths, farmland and bridges.'},{'id':'interiors','name':'Warm interiors','description':'12 floor, rug, wall and roof tiles.'},{'id':'dungeon','name':'Ancient stone','description':'12 dungeon tiles, stairs, grates and rune floors.'},{'id':'nature','name':'Nature & camps','description':'12 trees, bushes, rocks and outdoor props.'},{'id':'village','name':'Village & mechanisms','description':'18 buildings, furniture and interactive object states.'},{'id':'characters','name':'People & explorer','description':'4 NPCs and 16 directional walking frames.'},{'id':'items','name':'Pocket treasures','description':'12 item icons for keys, tools and rewards.'}]
def canvas(w=16,h=16):return Image.new('RGBA',(w,h),C[0])
def rect(im,box,c):ImageDraw.Draw(im).rectangle(box,fill=C[c])
def line(im,points,c,w=1):ImageDraw.Draw(im).line(points,fill=C[c],width=w)
def ellipse(im,box,c):ImageDraw.Draw(im).ellipse(box,fill=C[c])
def add(name,title,pack,im,kind='sprite',solid=False,water=False,collider=None,preset=None):
 rgba=list(im.getdata());lookup={v:k for k,v in C.items()};indices=[lookup[v] for v in rgba];a={'id':'eve-'+name,'name':title,'group':next(p['name'] for p in packs if p['id']==pack),'pack':pack,'kind':kind,'width':im.width,'height':im.height,'pixels':''.join(f'{i:02x}' for i in indices),'collider':collider or {'x':0,'y':0,'w':0,'h':0}};
 if kind=='tile':a.update(solid=solid,water=water)
 if preset:a['preset']=preset
 assets.append(a);im.save(png/(a['id']+'.png'));return a['id']
def texture(base,light,dark,salt=0):
 im=canvas();rect(im,(0,0,15,15),base)
 for y in range(16):
  for x in range(16):
   h=(x*17+y*29+x*y*3+salt*19)%97
   if h<3:im.putpixel((x,y),C[light])
   elif h==5:im.putpixel((x,y),C[dark])
 return im
# Outdoors: 24 tile assets.
for name,title,base,li,da in [('meadow','Soft meadow',1,28,25),('tall-grass','Tall grass',2,29,25),('dark-grass','Forest grass',25,2,24),('clover','Clover meadow',1,3,24)]:
 im=texture(base,li,da)
 if name=='tall-grass':
  for x,y in [(3,5),(10,12),(13,4)]:line(im,[(x-1,y),(x,y-2),(x,y)],29)
 if name=='clover':
  for x,y in [(4,5),(11,11)]:rect(im,(x,y,x+1,y+1),28)
 add(name,title,'outdoors',im,'tile')
for name,title,base,li,da in [('dirt','Packed dirt',4,5,6),('sand','Warm sand',5,14,4),('pebble','Pebble path',4,30,6),('cobble','Cobblestone path',8,9,7)]:
 im=texture(base,li,da)
 if name=='cobble':
  for y in [0,8]:
   for x in range(-4 if y else 0,16,8):rect(im,(x,y,x+6,y+6),9);line(im,[(x,y+6),(x+6,y+6)],7)
 add(name,title,'outdoors',im,'tile')
def waterTile(deep=False):
 im=texture(11 if deep else 12,12 if deep else 13,38,2)
 for x,y in [(1,4),(9,11)]:line(im,[(x,y),(x+4,y)],13 if not deep else 12)
 return im
add('water','Clear water','outdoors',waterTile(),'tile',water=True);add('deep-water','Deep water','outdoors',waterTile(True),'tile',water=True)
for edge,title in [('n','North shore'),('s','South shore'),('e','East shore'),('w','West shore'),('nw','Northwest shore'),('ne','Northeast shore'),('sw','Southwest shore'),('se','Southeast shore')]:
 im=waterTile()
 for y in range(16):
  for x in range(16):
   shore=('n' in edge and y<5) or ('s' in edge and y>10) or ('w' in edge and x<5) or ('e' in edge and x>10)
   rim=('n' in edge and y==5) or ('s' in edge and y==10) or ('w' in edge and x==5) or ('e' in edge and x==10)
   if shore:im.putpixel((x,y),C[1 if (x+y)%7 else 2])
   elif rim:im.putpixel((x,y),C[30])
 add('shore-'+edge,title,'outdoors',im,'tile',water=False)
for name,title in [('tilled','Tilled soil'),('seedlings','Seedling rows'),('planks','Garden planks'),('bridge-h','Bridge horizontal'),('bridge-v','Bridge vertical'),('fence','Fence rail')]:
 im=texture(31 if name in ['tilled','seedlings'] else 15,33,32)
 if name in ['tilled','seedlings']:
  for y in [3,9,15]:line(im,[(0,y),(15,y)],32)
  if name=='seedlings':
   for x in [3,11]:
    for y in [2,8]:line(im,[(x-1,y),(x,y+1),(x+1,y-1)],29);im.putpixel((x,y+2),C[25])
 else:
  for y in [3,7,11,15]:line(im,[(0,y),(15,y)],6)
  if name=='bridge-h':line(im,[(0,1),(15,1)],30);line(im,[(0,14),(15,14)],30)
  if name=='bridge-v':line(im,[(1,0),(1,15)],30);line(im,[(14,0),(14,15)],30)
  if name=='fence':
   im=texture(1,2,25);rect(im,(0,6,15,9),15);rect(im,(2,2,4,14),6);rect(im,(11,2,13,14),6);line(im,[(0,6),(15,6)],5)
 add(name,title,'outdoors',im,'tile',solid=name=='fence')
# Interior and dungeon tiles.
for name,title in [('wood-floor','Oak floor'),('dark-floor','Walnut floor'),('checker-floor','Tiled hall'),('stone-floor','Interior stone')]:
 im=texture(15 if name=='wood-floor' else 6 if name=='dark-floor' else 8,5,7)
 if name.endswith('floor') and name in ['wood-floor','dark-floor']:
  for y in [0,8,15]:line(im,[(0,y),(15,y)],32)
  line(im,[(5,0),(5,7)],6);line(im,[(12,8),(12,15)],6)
 if name=='checker-floor':
  for y in range(0,16,8):
   for x in range(0,16,8):rect(im,(x,y,x+7,y+7),9 if (x+y)%16 else 7)
 add(name,title,'interiors',im,'tile')
for edge in ['center','n','s','e','w']:
 im=canvas();rect(im,(0,0,15,15),40);rect(im,(2,2,13,13),39)
 if edge=='center':
  for x,y in [(3,3),(11,11)]:line(im,[(x,y+1),(x+1,y),(x+2,y+1),(x+1,y+2),(x,y+1)],41)
 else:
  if edge in ['n','s']:rect(im,(0,0 if edge=='n' else 12,15,3 if edge=='n' else 15),30)
  else:rect(im,(0 if edge=='w' else 12,0,3 if edge=='w' else 15,15),30)
 add('rug-'+edge,'Woven rug · '+edge,'interiors',im,'tile')
for name,title in [('plaster-wall','Cream plaster wall'),('brick-wall','Warm brick wall'),('roof','Terracotta roof')]:
 im=texture(14 if name=='plaster-wall' else 31,35,32)
 if name!='plaster-wall':
  for y in [0,8]:
   line(im,[(0,y),(15,y)],32)
   for x in [4 if y else 8,12 if y else 0]:line(im,[(x,y),(x,y+7)],32)
  if name=='roof':
   for y in [2,10]:line(im,[(0,y),(15,y)],33)
 add(name,title,'interiors',im,'tile',solid=name.endswith('wall'))
for name,title in [('dungeon-wall','Cut stone wall'),('moss-wall','Mossy stone wall'),('void','Deep shadow'),('flagstone','Flagstone floor'),('cracked-floor','Cracked flagstone'),('damp-floor','Damp stone'),('stairs-down','Steps down'),('stairs-up','Steps up'),('grate','Iron grate'),('torch-wall','Torch wall'),('rune-floor','Ancient rune'),('pillar-base','Pillar base')]:
 im=texture(7 if 'wall' in name else 8,9,38)
 if name=='void':rect(im,(0,0,15,15),26)
 if 'wall' in name:
  for y in [0,8,15]:line(im,[(0,y),(15,y)],38)
  line(im,[(8,0),(8,7)],38);line(im,[(4,8),(4,15)],38)
 if name=='moss-wall':rect(im,(0,0,6,2),25);rect(im,(12,9,15,12),24)
 if name=='cracked-floor':line(im,[(2,0),(6,5),(4,8),(10,13),(10,15)],38)
 if name=='damp-floor':ellipse(im,(1,9,11,13),12)
 if name.startswith('stairs-'):
  for y in range(2,16,3):rect(im,(2,y,13,y+1),10 if name.endswith('up') else 7)
 if name=='grate':
  for x in [3,7,11]:line(im,[(x,0),(x,15)],38)
  for y in [3,7,11]:line(im,[(0,y),(15,y)],38)
 if name=='torch-wall':rect(im,(7,6,9,13),15);ellipse(im,(5,2,11,8),45);ellipse(im,(7,3,9,6),44)
 if name=='rune-floor':line(im,[(4,3),(11,3),(7,7),(11,11),(4,11),(4,3)],13)
 if name=='pillar-base':rect(im,(2,2,13,13),7);rect(im,(4,4,11,11),9);line(im,[(3,2),(13,2)],10)
 add(name,title,'dungeon',im,'tile',solid=name in ['dungeon-wall','moss-wall','void','torch-wall','pillar-base'])
# Nature props, outlined silhouettes with small grounded colliders.
for name,title in [('oak','Old oak'),('pine','Tall pine'),('birch','Silver birch')]:
 im=canvas(32,48);ellipse(im,(6,42,27,47),26);rect(im,(14,18,18,44),6);line(im,[(16,22),(16,43)],15)
 if name=='pine':
  for cy,radius in [(10,8),(20,12),(30,15)]:ImageDraw.Draw(im).polygon([(16,cy-9),(16-radius,cy+7),(16+radius,cy+7)],fill=C[24]);line(im,[(16-radius+2,cy+6),(16,cy-5)],25)
 else:
  for box in [(1,9,24,31),(8,2,29,26),(4,3,23,23)]:ellipse(im,box,24 if name=='oak' else 25)
  ellipse(im,(5,5,22,19),28);ellipse(im,(14,12,29,27),2)
  if name=='birch':rect(im,(14,23,17,43),10);line(im,[(14,28),(17,28)],7);line(im,[(14,36),(16,36)],7)
 add(name,title,'nature',im,collider={'x':13,'y':36,'w':7,'h':10})
for name,title in [('bush','Leafy bush'),('berry-bush','Berry bush'),('rock','River rock'),('boulder','Large boulder'),('log','Fallen log'),('flower-clump','Wildflower clump'),('mushrooms','Mushroom cluster'),('reeds','Lakeside reeds'),('camp-flag','Camp pennant')]:
 im=canvas(32 if name in ['boulder','log','reeds'] else 16,24 if name in ['boulder','camp-flag'] else 16);w,h=im.size
 if 'bush' in name:ellipse(im,(1,3,14,14),24);ellipse(im,(2,2,10,11),28);ellipse(im,(7,6,14,13),25)
 if name=='berry-bush':
  for x,y in [(5,5),(10,7),(7,11)]:rect(im,(x,y,x+1,y+1),42)
 if name in ['rock','boulder']:ellipse(im,(1,3,w-2,h-2),7);ellipse(im,(3,2,w-5,h-6),9);line(im,[(4,5),(w-8,5)],10)
 if name=='log':rect(im,(2,5,29,13),6);ellipse(im,(1,4,8,14),5);ellipse(im,(3,6,6,12),15);line(im,[(9,7),(28,7)],15)
 if name=='flower-clump':
  for x,y in [(3,4),(10,2),(7,9)]:line(im,[(x,y),(x,y+5)],25);ellipse(im,(x-2,y-1,x+2,y+2),41);im.putpixel((x,y),C[44])
 if name=='mushrooms':
  for x,y in [(4,7),(10,10)]:rect(im,(x,y,x+1,y+4),14);ellipse(im,(x-3,y-3,x+3,y),19);im.putpixel((x,y-2),C[44])
 if name=='reeds':
  for x in [3,8,15,21,27]:line(im,[(x,15),(x+2,3)],28);rect(im,(x+1,1,x+3,5),15)
 if name=='camp-flag':line(im,[(4,2),(4,22)],6);ImageDraw.Draw(im).polygon([(5,3),(14,3),(11,8),(5,8)],fill=C[21]);line(im,[(5,3),(14,3)],22)
 solid=name in ['bush','berry-bush','rock','boulder','log'];add(name,title,'nature',im,collider={'x':2,'y':h-6,'w':w-4,'h':5} if solid else None)
# Village objects and alternate interaction states.
for name,title in [('chest','Oak chest'),('chest-open','Oak chest · open'),('door','Timber door'),('door-open','Timber door · open'),('gate','Iron gate'),('gate-open','Iron gate · open'),('lever','Stone lever'),('lever-on','Stone lever · latched'),('sign','Wooden sign'),('barrel','Store barrel'),('crate','Supply crate'),('well','Village well'),('table','Oak table'),('chair','Wooden chair'),('bed','Patchwork bed'),('lamp','Standing lamp'),('bookshelf','Bookcase'),('house','Small cottage')]:
 w,h=(64,64) if name=='house' else (32,32) if name in ['well','table','bed','bookshelf'] else (16,32) if name.startswith('door') or name.startswith('gate') else (16,24)
 im=canvas(w,h);preset=None
 if name.startswith('chest'):rect(im,(1,8,14,20),32);rect(im,(2,9,13,18),15);rect(im,(2,10,13,12),5);rect(im,(3,8,4,19),46);rect(im,(11,8,12,19),46);rect(im,(7,12,8,15),30)
 if name=='chest-open':rect(im,(2,4,13,8),15);rect(im,(2,9,13,13),26);rect(im,(5,10,10,12),45)
 if name.startswith('door'):
  rect(im,(1,0,14,31),6);rect(im,(3,2,12,29),15);line(im,[(7,2),(7,29)],32);rect(im,(10,16,11,17),30)
  if name=='door-open':rect(im,(3,2,12,29),26);rect(im,(3,2,5,29),15)
 if name.startswith('gate'):
  rect(im,(0,0,2,31),7);rect(im,(13,0,15,31),7)
  for x in [5,9]:line(im,[(x,2),(x,29)],8)
  line(im,[(2,8),(13,8)],9);line(im,[(2,22),(13,22)],9)
  if name=='gate-open':rect(im,(3,1,12,30),0);rect(im,(3,1,12,6),8)
 if name.startswith('lever'):rect(im,(2,15,13,21),7);rect(im,(4,14,11,18),9);line(im,[(8,15),(11 if name.endswith('on') else 4,6)],15,2);ellipse(im,(9 if name.endswith('on') else 2,3,13 if name.endswith('on') else 6,7),19 if not name.endswith('on') else 28)
 if name=='sign':rect(im,(7,10,9,23),6);rect(im,(0,2,15,12),32);rect(im,(1,3,14,10),5);line(im,[(4,6),(11,6)],6)
 if name=='barrel':ellipse(im,(1,4,14,22),32);rect(im,(2,6,13,19),15);line(im,[(2,9),(13,9)],7);line(im,[(2,17),(13,17)],7);ellipse(im,(2,2,13,7),5)
 if name=='crate':rect(im,(1,7,14,22),32);rect(im,(2,8,13,20),15);line(im,[(2,8),(13,20)],5);line(im,[(13,8),(2,20)],5)
 if name=='well':ellipse(im,(2,14,29,30),7);ellipse(im,(4,12,27,23),9);ellipse(im,(8,14,23,21),26);rect(im,(5,2,7,23),15);rect(im,(25,2,27,23),15);line(im,[(5,3),(27,3)],5,3);line(im,[(17,4),(17,17)],46)
 if name=='table':rect(im,(3,12,6,29),6);rect(im,(25,12,28,29),6);rect(im,(1,5,30,18),32);rect(im,(2,6,29,15),15);line(im,[(2,6),(29,6)],5)
 if name=='chair':rect(im,(3,4,12,15),6);rect(im,(4,5,11,12),15);rect(im,(2,14,13,18),5);rect(im,(3,18,4,23),6);rect(im,(11,18,12,23),6)
 if name=='bed':rect(im,(3,2,28,30),6);rect(im,(5,4,26,28),39);rect(im,(6,5,25,11),14);line(im,[(5,15),(26,15)],41);line(im,[(15,12),(15,28)],40)
 if name=='lamp':rect(im,(6,10,9,21),15);rect(im,(3,21,12,23),6);ellipse(im,(2,1,13,12),46);ellipse(im,(4,3,11,10),45);ellipse(im,(6,4,9,7),44)
 if name=='bookshelf':rect(im,(1,0,30,31),32);rect(im,(3,2,28,28),6)
 if name=='bookshelf':
  for y in [3,15]:
   for i,c in enumerate([21,19,28,39,20]):rect(im,(4+i*5,y,7+i*5,y+9),c);line(im,[(5+i*5,y+2),(6+i*5,y+2)],14)
  for y in [13,26]:rect(im,(2,y,29,y+1),5)
 if name=='house':
  rect(im,(9,25,54,60),6);rect(im,(11,27,52,57),14);rect(im,(10,54,53,58),8);ImageDraw.Draw(im).polygon([(5,28),(32,4),(59,28)],fill=C[32]);ImageDraw.Draw(im).polygon([(7,25),(32,5),(57,25)],fill=C[31]);
  for y in [13,18,23]:line(im,[(15 if y==13 else 9,y),(49 if y==13 else 55,y)],33)
  rect(im,(14,35,24,46),6);rect(im,(16,37,22,44),12);line(im,[(19,37),(19,44)],14);rect(im,(39,35,49,46),6);rect(im,(41,37,47,44),12);line(im,[(44,37),(44,44)],14);rect(im,(28,37,36,58),15);rect(im,(30,39,34,58),32)
 if name=='chest':preset={'kind':'chest','openSprite':'eve-chest-open','text':'You opened the chest.'}
 if name=='door':preset={'kind':'door','openSprite':'eve-door-open'}
 if name=='gate':preset={'kind':'door','openSprite':'eve-gate-open','switchOnly':True}
 if name=='lever':preset={'kind':'switch','openSprite':'eve-lever-on','targets':[]}
 if name=='sign':preset={'kind':'sign','text':'Welcome to your next adventure.'}
 solid=name in ['chest','door','gate','barrel','crate','well','table','chair','bed','bookshelf','house'];col={'x':2,'y':h-7,'w':w-4,'h':6} if solid else None
 if name in ['door','gate']:col={'x':1,'y':22,'w':14,'h':10}
 if name=='house':col={'x':9,'y':40,'w':46,'h':19}
 add(name,title,'village',im,collider=col,preset=preset)
# A new compact explorer, plus varied standing NPCs.
def person(dir='south',frame=0,shirt=21,hair=15):
 im=canvas(16,24);step=[0,1,0,-1][frame];side=dir in ['east','west'];cx=9 if dir=='east' else 6 if dir=='west' else 8
 rect(im,(cx-3,2,cx+3,8),26);rect(im,(cx-2,3,cx+2,8),17);rect(im,(cx-3,2,cx+3,4),hair);rect(im,(cx-3,3,cx-2,6),hair)
 if dir=='north':rect(im,(cx-2,4,cx+2,8),hair)
 elif side:im.putpixel((cx+2 if dir=='east' else cx-2,6),C[26])
 else:im.putpixel((cx-1,6),C[26]);im.putpixel((cx+1,6),C[26])
 rect(im,(4,9,11,19),26);rect(im,(5,9,10,17),shirt);line(im,[(5,10),(8,10)],22 if shirt==21 else 3);rect(im,(3+max(0,step),11,4+max(0,step),16),17);rect(im,(11-max(0,-step),11,12-max(0,-step),16),17);rect(im,(5,18,10,19),6)
 for x in [5+step,9-step]:rect(im,(x,20,x+1,22),23);rect(im,(x,22,x+2,23),26)
 return im
for dir in ['south','north','east','west']:
 for f in range(4):add('hero-'+dir+str(f),'Explorer · '+dir+' '+str(f+1),'characters',person(dir,f))
for name,title,shirt,hair in [('keeper','Grove keeper',15,10),('merchant','Market trader',28,6),('traveller','Trail traveller',21,32),('guard','Village guard',39,16)]:add(name,title,'characters',person(shirt=shirt,hair=hair),collider={'x':4,'y':17,'w':8,'h':7},preset={'kind':'npc','speaker':title,'pages':['Hello, traveller. This is a good place to begin your next story.']})
# Inventory icons.
for name,title in [('key','Bronze key'),('coin','Gold coin'),('heart','Heart'),('potion','Blue potion'),('sword','Short sword'),('gem','Azure gem'),('food','Trail bread'),('scroll','Parchment scroll'),('shield','Round shield'),('ore','Iron ore'),('wood','Wood bundle'),('flower','Meadow flower')]:
 im=canvas()
 if name=='key':ellipse(im,(2,2,8,8),46);ellipse(im,(4,4,6,6),0);line(im,[(7,7),(13,13)],30,2);line(im,[(10,11),(12,9)],30)
 if name=='coin':ellipse(im,(3,2,12,13),46);ellipse(im,(4,3,11,11),30);line(im,[(7,5),(7,9),(9,9)],44)
 if name=='heart':ImageDraw.Draw(im).polygon([(2,4),(5,2),(8,5),(11,2),(14,4),(14,7),(8,13),(2,7)],fill=C[19]);line(im,[(4,4),(5,4)],43)
 if name=='potion':rect(im,(6,1,9,4),15);ellipse(im,(3,4,12,14),23);ellipse(im,(4,5,11,13),12);rect(im,(5,8,10,12),13);line(im,[(5,6),(5,9)],10)
 if name=='sword':line(im,[(3,13),(6,10)],6,2);line(im,[(5,9),(8,12)],46,2);ImageDraw.Draw(im).polygon([(6,9),(11,2),(14,1),(13,4),(8,11)],fill=C[10]);line(im,[(8,9),(13,3)],9)
 if name=='gem':ImageDraw.Draw(im).polygon([(8,1),(13,5),(11,12),(8,15),(4,11),(3,5)],fill=C[12]);ImageDraw.Draw(im).polygon([(8,2),(10,5),(8,12),(5,5)],fill=C[13]);im.putpixel((7,4),C[10])
 if name=='food':ellipse(im,(2,4,14,12),6);ellipse(im,(2,3,13,10),5);line(im,[(5,4),(4,7)],14);line(im,[(9,4),(8,7)],14)
 if name=='scroll':rect(im,(4,2,12,13),14);ellipse(im,(2,1,7,5),5);ellipse(im,(9,11,14,15),5);line(im,[(6,6),(10,6)],6);line(im,[(6,9),(10,9)],6)
 if name=='shield':ellipse(im,(2,1,14,14),7);ellipse(im,(3,2,13,13),15);ellipse(im,(6,5,10,10),9);line(im,[(8,3),(8,5)],5)
 if name=='ore':ImageDraw.Draw(im).polygon([(2,12),(3,5),(9,2),(13,6),(14,12)],fill=C[7]);ImageDraw.Draw(im).polygon([(3,8),(7,4),(10,6),(8,10)],fill=C[9])
 if name=='wood':
  for y in [5,9,13]:line(im,[(3,y),(13,y-2)],15,3);ellipse(im,(1,y-2,5,y+1),5)
 if name=='flower':line(im,[(8,7),(8,14)],25);line(im,[(8,11),(11,9)],28);ellipse(im,(4,1,11,8),41);ellipse(im,(6,3,9,6),45)
 add(name,title,'items',im)
# Portable, four-layer stamps use asset IDs.
stamps=[]
def stamp(id,name,w,h,fn):
 layers={k:[] for k in ['base','detail','collision','water']}
 for y in range(h):
  for x in range(w):
   tile,blocked,wet=fn(x,y);layers['base'].append('eve-'+tile);layers['detail'].append(None);layers['collision'].append(int(blocked));layers['water'].append(int(wet))
 stamps.append({'id':'eve-stamp-'+id,'name':name,'width':w,'height':h,'layers':layers})
def pond(x,y):
 edge=('n' if y==0 else 's' if y==4 else '')+('w' if x==0 else 'e' if x==5 else '');return ('shore-'+edge if edge else 'water',False,not edge)
stamp('pond','Small pond · 6 × 5',6,5,pond)
stamp('garden','Vegetable patch · 4 × 3',4,3,lambda x,y:('seedlings' if y%2==0 else 'tilled',False,False))
stamp('cobbles','Stone plaza · 4 × 4',4,4,lambda x,y:('cobble',False,False))
stamp('bridge','Vertical bridge · 3 × 6',3,6,lambda x,y:('bridge-v' if x==1 else 'water',False,x!=1))
stamp('room','Stone room · 10 × 8',10,8,lambda x,y:('dungeon-wall' if x==0 or x==9 or y==0 or y==7 and x not in [4,5] else 'flagstone',x==0 or x==9 or y==0 or y==7 and x not in [4,5],False))
stamp('floor','Cottage floor · 8 × 6',8,6,lambda x,y:('wood-floor',False,False))
stamp('rug','Woven rug · 3 × 3',3,3,lambda x,y:('rug-n' if y==0 else 'rug-s' if y==2 else 'rug-w' if x==0 else 'rug-e' if x==2 else 'rug-center',False,False))
stamp('yard','Fenced yard · 8 × 6',8,6,lambda x,y:('fence' if (x==0 or x==7 or y==0 or y==5) and not(y==5 and x in [3,4]) else 'meadow',(x==0 or x==7 or y==0 or y==5) and not(y==5 and x in [3,4]),False))
bundle={'format':'evengine-builtins','version':1,'palette':palette,'packs':packs,'assets':assets,'stamps':stamps}
(root/'library.json').write_text(json.dumps(bundle,indent=2));(ui/'builtins-data.js').write_text('// Original pixel art, generated by editor/assets/build_assets.py.\n(function(r){r.EveBuiltins='+json.dumps(bundle,separators=(',',':'))+';})(typeof window!=="undefined"?window:globalThis);\n')
for pack in packs:
 rows=[a for a in assets if a['pack']==pack['id']];sheet=canvas(8*64,math.ceil(len(rows)/8)*80)
 for i,a in enumerate(rows):
  image=Image.open(png/(a['id']+'.png'));sheet.paste(image,(i%8*64+(64-a['width'])//2,i//8*80+8),image)
 sheet.save(root/(pack['id']+'-sheet.png'))
(root/'ASSET-NOTES.md').write_text('# EveNGINE built-in artwork\n\n110 original pixel-art assets (48 tiles and 62 sprites), plus eight four-layer map stamps. Artwork was created for this release; no third-party asset packs are included. Individual transparent PNGs are in `png`; pack contact sheets and the palette-indexed `library.json` are also included.\n\nTiles are 16 × 16; sprite sizes are 16 × 16 through 64 × 64. The pack contact sheets use 64 × 80 slots for browsing, not uniform 16 × 16 tile-sheet import. Use individual PNGs, the built-in editor library or the JSON metadata. Editing imported assets does not change the built-in originals.\n\nThe source generator uses Pillow only for rebuilding the artwork; the editor and game require no Python.\n')
assert len(assets)==110,len(assets);assert sum(a['kind']=='tile' for a in assets)==48
print('Built',len(assets),'assets and',len(stamps),'stamps')
