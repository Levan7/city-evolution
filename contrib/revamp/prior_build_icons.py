from pathlib import Path
from lxml import etree as E
from PIL import ImageFont
import sys,html
sys.path.insert(0,str(Path(__file__).resolve().parent))
from svg_render import render
base=Path(__file__).resolve().parents[1];src=Path(sys.argv[1]) if len(sys.argv)>1 else Path('claude-review/city-evolution-Preview-8-source/data/images/readable');out=base/'assets/icons';over=base/'overlay/images/readable';over.mkdir(parents=True,exist_ok=True)
S={
'house':'<path fill="#f6dfb5" d="M8 24L24 16 40 24V40L24 47 8 39Z"/><path fill="#c57250" d="M3 23L24 6 45 23 24 33Z"/><path fill="#244e58" d="M20 32h8v13h-8Z"/>',
'factory':'<path fill="#d7976b" d="M5 27L17 19V27L29 19V27L43 19V44H5Z"/><path fill="#abcac8" d="M8 6h7v19H8Z"/><path d="M11 3q-8-8 2-8"/><path fill="#254e5d" d="M11 33h6v6h-6m12-6h6v6h-6m12-6h5v6h-5"/>',
'water':'<path fill="#69cbd5" d="M24 3C21 12 8 22 8 31a16 16 0 0032 0C40 22 27 12 24 3Z"/><path d="M15 29q-2 9 7 10"/>',
'book':'<path fill="#f8e6bc" d="M3 9q12-5 21 3 10-8 21-3v30q-11-3-21 3-9-6-21-3Z"/><path d="M24 12v30M9 18h8m-8 7h8m14-7h8m-8 7h8"/>',
'bolt':'<path fill="#ffca68" d="M27 2L8 29h15l-3 17 21-29H26Z"/>',
'road':'<path fill="#859d9d" d="M14 2h20l10 44H4Z"/><path stroke="#fff0c8" stroke-dasharray="5 4" d="M24 4v38"/>',
'rail':'<path d="M15 2L7 46M33 2l8 44M12 11h24M10 21h28M8 32h32M6 43h36"/>',
'park':'<path fill="#b27b53" d="M20 27h8v20h-8Z"/><path fill="#83b576" d="M24 2L8 19h8L3 32h42L32 19h8Z"/>',
'health':'<path fill="#f2e1c4" d="M5 9h38v35H5Z"/><path fill="#d57266" stroke="none" d="M20 15h8v7h7v8h-7v7h-8v-7h-7v-8h7Z"/>',
'fire':'<path fill="#df8954" d="M24 1c5 13 19 19 18 31-1 20-35 20-36 0 0-10 6-18 13-23-2 15 3 11 5-8Z"/><path fill="#ffe1a0" d="M25 24c15 12 7 24-1 24-12 0-15-11 1-24Z"/>',
'wind':'<circle cx="24" cy="18" r="4" fill="#ffe2a0"/><path d="M24 22v24M24 14V1M20 20L4 29M28 20l16 9"/>',
'warehouse':'<path fill="#edd6b5" d="M4 22L24 9l20 13v23H4Z"/><path fill="#588b84" d="M1 21L24 5 47 21 42 26 24 14 6 26Z"/><path fill="#30505c" d="M13 28h22v17H13Z"/><path d="M15 32h18m-18 5h18"/>',
'cafe':'<path fill="#efe1c4" d="M5 9h38v35H5Z"/><path fill="#7fb29b" d="M3 10h42v9H3Z"/><path fill="#ae7150" d="M12 25h22v13H12Z"/><path d="M34 27h6v7h-6M11 42h25"/>',
'rocket':'<path fill="#eadcc2" d="M24 2Q9 12 15 34h18Q39 12 24 2Z"/><circle fill="#73b8bc" cx="24" cy="18" r="5"/><path fill="#cb795d" d="M15 27L6 40l12-6m15-7 9 13-12-6"/><path stroke="#ffc466" d="M20 40v7m8-7v7"/>',
'port':'<circle cx="24" cy="8" r="5"/><path d="M24 13v29M13 21h22M5 28q0 15 19 15t19-15M5 28l-3 8m41-8 3 8"/>',
'pointer':'<path fill="#8cc5bd" d="M8 3v37l10-10 9 16 8-4-10-15 14-2Z"/>',
'bulldoze':'<path fill="#dfb05d" d="M8 21h27v18H8Z"/><path fill="#8f9fa3" d="M32 37L45 27v16H31Z"/><path fill="#355366" d="M11 11h14v16H11Z"/><path d="M4 42h30"/>',
'evacuate':'<path d="M8 42V10h18v8M8 42h19M20 28h24m-8-8 8 8-8 8"/>',
'recycle':'<path d="M15 12l9-9 11 19m-1-10 1 10-10-2M39 25l7 15H24m9 7-9-7 8-6M19 42H3l9-20m-9 4 9-4 4 9"/>',
'coal':'<path fill="#789093" d="M5 29L16 10 30 8 44 27 37 43 13 44Z"/><path d="M16 10l6 18 22-1M22 28l-9 16"/>',
'ore':'<path fill="#bd8b6c" d="M5 29L16 10 30 8 44 27 37 43 13 44Z"/><path stroke="#f0c578" d="M13 25l7-5m6 15 9-6m-3-15 4 6"/>',
'farm':'<path d="M7 40l34-16M5 30l30-15M12 46l31-15"/><path fill="#98b77b" d="M24 27V5m0 8c-12-12-15 0 0 6m0-6c12-12 15 0 0 6"/>',
'well':'<ellipse fill="#a4c9ce" cx="24" cy="35" rx="16" ry="9"/><path d="M8 35v9q16 8 32 0v-9M12 34V12m24 22V12M5 13L24 2l19 11M24 12v19"/>',
'solar':'<path fill="#66a6b7" d="M10 17h30l5 20H5Z"/><path d="M20 18l-2 19m11-19 2 19M8 27h34M16 38v8m18-8v8"/><circle fill="#ffd078" cx="9" cy="6" r="5"/>',
'sports':'<circle fill="#d3a36b" cx="24" cy="24" r="20"/><path d="M5 22h38M24 4v40M10 9q23 16 28 28M37 9Q14 25 10 38"/>',
'monument':'<path fill="#c5d1c7" d="M24 2L16 11v26h16V11Z"/><path fill="#728e91" d="M10 37h28v9H10Z"/>',
'pottery':'<path fill="#d78e62" d="M18 4h12v8l10 11q8 23-16 23T8 23l10-11Z"/><path d="M16 28h16M15 35h18"/>',
'blacksmith':'<path fill="#bbc7bf" d="M4 12h40l-7 10H26v11h9v10H11V33h9V22H10Z"/>',
'mill':'<path d="M24 21v25M9 44h30"/><path fill="#9cc6c1" d="M24 21L5 5l17-3 2 19 19-16 3 17-22-1 19 20-17 4-2-24L5 41 2 25Z"/>',
'university':'<path fill="#e2d3ab" d="M3 13L24 2l21 11ZM6 42h36v5H6Z"/><path d="M11 17v21m13-21v21m13-21v21M5 17h38"/>',
'substation':'<path fill="#9bc4bc" d="M5 12h38v32H5Z"/><path fill="#ffd17a" d="M26 15L14 29h9l-1 12 12-18h-9Z"/><path d="M10 4v9m10-9v9m10-9v9m10-9v9"/>',
'powerline':'<path d="M24 3L12 46m12-43 12 43M10 12h28M6 23h36M16 33h16M5 9v9m38-9v9"/>',
'landfill':'<path fill="#a38b70" d="M3 43l8-22 10 8 9-19 15 33Z"/><path stroke="#93b98a" d="M9 42l9-8 8 8m-1-17 6 7 7-5"/>',
'commune':'<circle fill="#e9c79c" cx="14" cy="10" r="6"/><circle fill="#e9c79c" cx="34" cy="10" r="6"/><path fill="#7fb3a4" d="M4 44V23q10-14 20 0 10-14 20 0v21Z"/>',
}
keys=[('Residence','house'),('Warehouse','warehouse'),('FoodHall','cafe'),('TechCampus','university'),('Cafe','cafe'),('Market','warehouse'),('Farm','farm'),('Waterwell','well'),('Water','water'),('LIndustry','factory'),('HIndustry','factory'),('Industry','factory'),('School','book'),('University','university'),('Health','health'),('Fire','fire'),('Sports','sports'),('Park','park'),('Monument','monument'),('Track','road'),('Street','road'),('Transport','road'),('Rail','rail'),('Port','port'),('Rocket','rocket'),('CoalPS','factory'),('SolarPS','solar'),('Substation','substation'),('Windmill','wind'),('PowerLine','powerline'),('Power','bolt'),('Commune','commune'),('Coal','coal'),('Ore','ore'),('Mining','ore'),('Tip','landfill'),('Recycle','recycle'),('Pottery','pottery'),('Blacksmith','blacksmith'),('Mill','mill'),('BullDoze','bulldoze'),('Evacuate','evacuate'),('Pointer','pointer'),('Evolution','warehouse')]
for p in src.glob('BP*.svg'):
 t=E.parse(str(p));label=''.join(t.getroot().itertext());key=next((v for k,v in keys if k in p.stem),'house');symbol=S[key]
 if 'Residence' in p.stem and p.stem.startswith('BPM'):
  number=int(p.stem.replace('BPMResidence','').replace('Button',''));levels=(number-1)%3+1;roof='#c57250' if number<=3 else '#5b9b9c';top=30-levels*7
  symbol=f'<path fill="#f6dfb5" d="M7 {top+8}L24 {top} 41 {top+8}V41L24 47 7 40Z"/><path fill="{roof}" d="M4 {top+8}L24 {top-7} 44 {top+8} 24 {top+18}Z"/>'
  for level in range(levels):
   y=40-level*7;symbol+=f'<path stroke="#407583" stroke-width="3" d="M12 {y-3}l6 3m12-1 6-3"/>'
 if 'TechCampus' in p.stem:symbol='<path fill="#82b5b6" d="M5 18L24 9 43 18V40L24 47 5 38Z"/><path d="M24 20v23M12 22v12m24-12v12M16 6h16M24 2v8"/>'
 if 'HIndustry' in p.stem:symbol+='<path fill="#adc9c4" d="M28 3h6v20h-6Z"/>'
 if 'FoodHall' in p.stem:symbol+='<path d="M9 1v8m4-8v8m4-8v8M13 9v8M36 2v14"/>'
 if 'Track' in p.stem:symbol=symbol.replace('#859d9d','#b19670').replace('stroke-dasharray="5 4"','stroke-dasharray="1 7"')
 if 'Market' in p.stem:symbol+='<path fill="#8ab7a2" d="M3 22h42v7H3Z"/>'
 if 'CoalPS' in p.stem:symbol+='<path fill="#ffd17a" stroke="none" d="M27 21L18 34h8l-2 10 12-16h-9Z"/>' 
 standalone=f'<svg xmlns="http://www.w3.org/2000/svg" width="48" height="48" viewBox="0 0 48 48"><g fill="none" stroke="#e6cb96" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">{symbol}</g></svg>'
 q=out/(p.stem+'.svg');q.write_text(standalone);render(q,q.with_suffix('.png'),48,48)
 category=not p.stem.startswith('BPM');w,h=(144,48) if category else (72,64);size=12 if category else 11;avail=84 if category else 68
 while ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',size).getlength(label)>avail:size-=1
 geom='translate(5,3) scale(.85)' if category else 'translate(16,1) scale(.83)'
 text=f'<text x="{53 if category else 36}" y="{29 if category else 59}" text-anchor="{ "start" if category else "middle"}" fill="#fff6df" font-family="DejaVu Sans" font-size="{size}">{html.escape(label)}</text>'
 toolbar=f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}"><g transform="{geom}" fill="none" stroke="#e6cb96" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">{symbol}</g>{text}</svg>'
 q=over/(p.stem+'.svg');q.write_text(toolbar);render(q,q.with_suffix('.png'),w,h)
print('Rebuilt',len(list(out.glob('*.png'))),'semantic toolbar icons, with native SVG sources and labeled overlays.')
