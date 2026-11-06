from pathlib import Path
import struct,zipfile,hashlib,json
r=Path('preview15-work');p=r/'City Evolution.exe';b=p.read_bytes();pe=struct.unpack_from('<I',b,0x3c)[0];n=struct.unpack_from('<H',b,pe+6)[0];opt=pe+24;osz=struct.unpack_from('<H',b,pe+20)[0];assert struct.unpack_from('<H',b,opt)[0]==0x20b;assert struct.unpack_from('<H',b,opt+68)[0]==2
rva,size=struct.unpack_from('<II',b,opt+112+16);base=None
for i in range(n):
 sec=opt+osz+i*40;vsize,va,rawsize,raw=struct.unpack_from('<IIII',b,sec+8)
 if va<=rva<va+max(vsize,rawsize):base=raw+rva-va
assert base is not None
named,ids=struct.unpack_from('<HH',b,base+12);types=set()
for i in range(named+ids):
 name,offset=struct.unpack_from('<II',b,base+16+i*8)
 if not name&0x80000000:types.add(name)
assert {3,14,16}<=types,types
zpath=r/'CityEvolution-Preview-15-Windows-x64.zip'
with zipfile.ZipFile(zpath) as z:
 assert z.testzip() is None,'ZIP CRC mismatch';prefix='CityEvolution-Preview-15-Windows-x64/'
 assert z.read(prefix+'City Evolution.exe')==b
 assert prefix+'Game/bin/lincity-ng.exe' in z.namelist()
 assert prefix+'Play.cmd' not in z.namelist()
 for name in ['autonomy','street-routes','flu','services','community','mayor','transit','city-evolution','audio']:assert prefix+'Game/diagnostics/'+name+'-checks.exe' in z.namelist()
 for name in ['buttonpanel','minimap','pbar','publicservices']:
  assert 'Preview 14' not in z.read(prefix+'Game/share/lincity-ng/gui/'+name+'.xml').decode()
print('PASS Windows ZIP CRC, root GUI launcher, real icon/group/version resources, embedded game and nine diagnostics; no Play.cmd')
report={p.name:{'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in [r/'CityEvolution-Preview-15-Linux-x86_64.AppImage',zpath]};(r/'binary-checksums.json').write_text(json.dumps(report,indent=2))
