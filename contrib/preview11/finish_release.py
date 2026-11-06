from pathlib import Path
from lxml import etree as E
import json,hashlib,shutil,zipfile,difflib,subprocess
W=Path(__file__).resolve().parent;S=W/'CityEvolution-Preview-11-Source';G=W/'CityEvolution-Preview-11';OUT=W.parent/'deliverables'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
if (S/'CHECKPOINT.txt').exists():(S/'CHECKPOINT.txt').unlink()
for n in ['community','transit']:
 text=(S/'validation'/f'preview11-{n}.log').read_text();assert 'PASS' in text and not any(x in text for x in ['invalid group','FAIL'])
runtime=json.loads((S/'validation/preview11-runtime-audit.json').read_text());assert not runtime['missing_dlls'] and not runtime['missing_symbols']
changelog="""# City Evolution — Preview 11
4 November 2026. A fork of LinCity-NG.

This release contains a newly compiled native Windows executable and integrates the church/traffic/UI work with the later request for lower, more varied buildings.

## Artwork and road visibility

The screenshots first showed the original hide-tall-buildings view: colored diamonds replace structures larger than one tile while a one-tile bus shelter remains visible. H toggles that view. The subsequent screenshots confirmed the models were present and showed that the previous palace-like replacements were oversized and repetitive.

- Restored low, utilitarian silhouettes based on the original game.
- Three forestry-commune appearances: small huts, wood/charcoal work areas and trees instead of repeated large courtyard factories.
- Twelve housing appearances: two variants for each of the six tiers. Timber cottages, stone houses, brick terraces, white/pastel bungalows and compact three-storey apartments have different materials and colours.
- Distinct low pottery kilns, a forge, a brick textile mill and market stalls.
- Farms are mainly crop fields with a small barn. Crop-state slots remain.
- All replacement building sprites are drawn at 80% of their prepared model scale over original-size native grass lots. This reduces their visual area to 64% without changing construction footprint, costs, commodity rules or road connectivity.
- Building thumbnails now show the current, smaller in-game models.
- Commune/housing variations are derived from map position; fixed-coordinate save/load retains the appearance without new save fields.
- The original animation slot counts remain. Most animated replacements use three painted poses in the original timing slots. Smoke sprites and their movement offsets are scaled with the factories.

The exact export dimensions, bindings and scale factors are in art/preview11/scale-metrics.json and compact-resource-layout.json. Art masters and generation prompts are supplied. The contact sheet is art/preview11/compact-review.jpg. The renderer still uses ordinary isometric depth ordering: a building can cover some ground behind it. This release reduces that obstruction; it does not promise every road segment is visible from every view.

## Menus and controls

- Restored the custom generated riverside city background, with menu buttons in its shaded left area.
- Restored one horizontal row of pause, green play, fast and fastest buttons, using the existing created dark teal theme.
- Mayor is at the top right, with Transport report directly below.
- Removed construction buttons and their callbacks from the transport report. Construction stays in the left categories.
- Kept 15 left categories and all previous 51 tool definitions; church is tool 52 in Civic.
- Categories stay openable when their default tool is locked; unavailable individual buildings remain disabled with requirement tooltips.
- Selecting tunnel/elevated construction from the sidebar automatically selects the corresponding transport layer view.
- Models reset visible when a world opens. A labelled button beneath Transport report restores model visibility; H remains the original toggle.
- Decorative details hide with their parent buildings. The long warehouse explanation is shortened to fit the fixed statistics panel.

## Horses, carriages and cars

The shared policy is src/lincity/traffic_era.hpp. Elapsed time alone does not unlock cars.

| Technology | Motor traffic share |
| --- | --- |
| Below 10% | 0%; horses and carriages |
| At 10% | 1% |
| 10% to 30% | Gradual deterministic transition |
| 30% or above | 100% |

The policy applies to native vehicles, newer street actors, parked warehouse detail and representative bus-route vehicles. Early bus routes display horse carriages. Parked trucks become carts before motorisation. There are four horse views and four carriage views. The native vehicle group retains 16 slots, reusing carriage views for turns/slopes. Vehicles move along built streets/tracks; empty wilderness has no ambient traffic.

This changes visual traffic eras, not commodity logistics, timetables, capacity or transport build technology requirements. Rail retains its own prerequisites. Horses/carts used as parked building details remain decorative. The moving sprites are not a detailed animal/driver economy or a full leg/wheel animation system.

## Georgian Orthodox Christian church

Find it in Civic. The model has Georgian stonework, a conical teal roof, a bell tower and visible Christian crosses.

| Rule | Value |
| --- | ---: |
| Construction | $6,000; 3×3 footprint |
| Technology/electricity requirement | None |
| Daily input | 2 labor + 1 goods |
| Daily upkeep, including closed days | $8 |
| Learning while operating | +1 technology point/day |
| Spiritual wellbeing radius | 8 Manhattan-distance tiles from footprint |
| Nearby residence morale | +8, non-stacking |
| Target city approval | Up to +6 according to covered housing share |

The church is an optional game amenity. No separate religious penalty applies when it is absent. It does not produce formal school/university graduates or satisfy their skilled-workforce prerequisites. Closed churches grant no learning or coverage benefit.

Operating counters and a separate expenditure ledger are saved. Active service is recalculated from supplies on the next simulated day. The compact Other Costs screen combines Schools/Church while retaining separate saved accounts. City Hall shows the homes covered and number of operating churches.

Old saves remain loadable. Saves containing churches require Preview 11 or later.

## Native code and audit coverage

New/changed code covers group 62 and group count 63, church registration/module, input consumption, education, coverage, residence morale, approval, spending, yearly totals, save/load, traffic progression, model visibility, report callbacks, layer selection, browsable categories, stable art variants and scaled smoke offsets. The game and four Windows simulation diagnostics are rebuilt.

The audit materials include full current engine and GUI source, tests, build notes, binary hashes, import/export results, active resource mappings, art previews and two textual patch stages:
1. The inherited Changes-from-Claude.patch records the earlier Claude Preview 8 → Preview 10 work.
2. Preview10-to-Preview11.patch records this release's changes.

The full Source ZIP supplies all binary artwork and third-party source. The focused hash report records binary changes as well. The previous baseline comparison is preserved as history; the combined current index records original-before and current-after hashes for changes since the supplied Claude archive. Patches alone are not a playable update.

## Verification

- Windows x64 UCRT game and four diagnostics compiled successfully.
- Native real-engine community tests passed for traffic thresholds, monotonicity, seed mixes, church construction/costs, staffing, upkeep, learning, coverage, wellbeing, save/load, closure without goods and demolition.
- Stable placement-based commune variant assignment passed.
- All four transit modes passed native regression tests: ridership, coverage, fares, upkeep, save/load, route cuts, staffing, interchange and independent layers.
- Source/package GUI style checks passed in both NLS branches.
- Asset checks passed: 52 tools, 15 categories, 42 table layouts, all original native animation slot counts, three commune variants, twelve housing appearances and 95 rescaled building frames.
- Windows PE dependencies and imported symbols passed, including the dynamic PNG/Vorbis/font libraries used by the shipped files.
- Release ZIP CRCs and per-file SHA-256 entries are checked.

Headless tests use inert display/audio adapters and do not test graphical interaction. Windows startup, final rendering alignment and audio playback have not been play-tested here. Isometric screenshots/contact sheets and static layout checks are not a Windows play-through.

## Remaining limits

Terrain, bridges, network tiles and some effects still use upstream art. Many working-building loops reuse three new poses rather than repainting every original pose. The complete source preserves original/input images for reproduction; the playable package removes unreferenced tile images and has one ordered binding per active resource.

## Install

Extract the complete Windows ZIP into a new writable folder and run Play.cmd. Keep personal saves backed up before copying them into PortableData. Do not mix old and new DLL folders. Verify-Community.cmd runs the church/traffic diagnostic. For startup failures, Diagnose-Startup.cmd records diagnostics; PortableData/launch.log contains startup output.
"""
(S/'CHANGELOG-Preview-11.md').write_text(changelog);(G/'CHANGELOG-Preview-11.md').write_text(changelog);(OUT/'CityEvolution-Preview-11-Changelog.md').write_text(changelog)
(S/'README.md').write_text("""# City Evolution — Preview 11
A fork of LinCity-NG.

Complete engine source/data for the rebuilt Preview 11 release. Read CHANGELOG-Preview-11.md for behavior, validation and limits.

The changes integrate compact/varied building art, a Georgian Orthodox church, horse-to-motor traffic progression, restored menu/speed controls and a right-side report-only transport window.

Build with CMake as described in PREVIEW-9.md; older release notes describe historical revisions. Use ENABLE_NLS=OFF and LINCITYNG_EVOLUTION_CHECKS=ON for diagnostics. Targets: lincity-ng, community-checks, transit-checks, mayor-checks, city-evolution-checks. Bundled fmt/libxml++ must use the same C++ compiler as the game.

Tests: python3 tests/preview11_assets_check.py and python3 tests/gui_style_check.py. Native community/transit results and Windows import audit are in validation/. Windows graphical play-through remains unverified.

Audit: the earlier Changes-from-Claude.patch plus Preview10-to-Preview11.patch; full current source and binary artwork are included here. The baseline/current hash index is validation/current-changes-from-claude.json. Raw new art and export information are in art/preview11.
""")
(G/'START-HERE.txt').write_text("""CITY EVOLUTION - PREVIEW 11
A fork of LinCity-NG.

Extract the ENTIRE ZIP into a NEW writable folder. Run Play.cmd.
Keep personal saves backed up. Do not mix old and new DLL directories.

Smaller, more varied building models are integrated.
Mayor and Transport report are at the TOP RIGHT.
Construction is in the LEFT categories; the church is in Civic.
Pause / play / fast / fastest form one horizontal row at lower left.
The custom riverside city background is restored.

If you see colored building footprints, press H or click the labelled
building-model button below Transport report to show the models.

Traffic starts with horses/carriages on built streets and tracks.
Motor vehicles begin at 10% technology, reaching 100% at 30%.
Churches need 2 labor and 1 goods/day and cost $8/day.
They provide community learning and nearby morale benefits.

Native simulation tests and asset/import checks passed.
Windows graphical play-through remains unverified.
Read CHANGELOG-Preview-11.md for exact values and remaining limits.
Verify-Community.cmd runs the church/traffic diagnostic on Windows.
For startup issues run Diagnose-Startup.cmd and include its output
along with PortableData/launch.log.
""")
hist=G/'Docs/History';hist.mkdir(parents=True,exist_ok=True)
for n in ['CHANGELOG-Preview-10.md','HOTFIX-Preview-9-Menu.txt']:
 if (G/n).exists():shutil.move(str(G/n),hist/n)
val=G/'Docs/Validation';val.mkdir(parents=True,exist_ok=True)
for p in (S/'validation').glob('preview11-*'):shutil.copy2(p,val/p.name)
shutil.copy2(S/'art/preview11/compact-review.jpg',G/'Docs/Compact-art-review.jpg')
# Persist exact production scripts and build dependency hashes for review/reproduction.
c=S/'contrib/preview11';c.mkdir(exist_ok=True)
for p in W.glob('*.py'):shutil.copy2(p,c/p.name)
for p in W.glob('shared-configs/*/*.cmake'):
 q=c/'release-shared-configs'/p.parent.name/p.name;q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
(c/'BUILD-NOTES.md').write_text("""The exported data is complete; normal engine builds do not rerun the asset scripts. The Python files record one-time production transforms against a Preview 10 workspace. Paths in those transforms identify that workspace.

Windows release: LLVM-MinGW 20261104 UCRT x86_64, Clang 23.1.5; bundled C++ fmt/libxml++ built with the game. CMake 4.4.4 and Ninja 1.13.2. The minimal shared SDL import configs are release environment wiring, not fake implementation libraries: their IMPORTED_IMPLIB and IMPORTED_LOCATION point to the actual MSYS2 import libraries/DLLs. For a normal full SDK, use its standard package configs.

Native headless tests: GNU 13.3.0, host libxml2 2.9.14, matching Ubuntu development/ICU headers. SDL interfaces were declarations only; display/audio use the existing inert test adapters. Simulation code is real. No Windows graphics claim is made.
""")
(c/'build-downloads.json').write_text(json.dumps([{'name':p.name,'sha256':sha(p),'bytes':p.stat().st_size} for p in sorted((W/'cache').iterdir()) if p.is_file()],indent=2))
# Compare exact original Preview 10 ZIP bytes with current tree. No extraction needed.
baseline=W.parent/'latest-recovery/CityEvolution-Preview-10-Source.zip'
excluded={'SHA256SUMS.txt','Preview10-to-Preview11.patch','validation/preview10-to-preview11.json','validation/current-changes-from-claude.json'}
current={str(p.relative_to(S)):p for p in S.rglob('*') if p.is_file() and '__pycache__' not in p.parts}
with zipfile.ZipFile(baseline) as z:
 entries={str(Path(*Path(n).parts[1:])):n for n in z.namelist() if not n.endswith('/')}
 rows=[];patch=[]
 previous=json.loads(z.read(entries['validation/changes-from-claude.json']))
 old_rows={x['path']:x for x in previous['files']};combined=[]
 for name in sorted(set(entries)|set(current)|set(old_rows)):
  if name in excluded:continue
  left=z.read(entries[name]) if name in entries else None;p=current.get(name);right=p.read_bytes() if p else None
  before=hashlib.sha256(left).hexdigest() if left is not None else None;after=hashlib.sha256(right).hexdigest() if right is not None else None
  original=old_rows[name]['before_sha256'] if name in old_rows else before
  if original!=after:combined.append({'path':name,'before_sha256':original,'after_sha256':after,'status':'added' if original is None else 'removed' if after is None else 'modified'})
  if before==after:continue
  rows.append({'path':name,'before_sha256':before,'after_sha256':after,'status':'added' if left is None else 'removed' if right is None else 'modified'})
  try:
   l=left or b'';r=right or b''
   if b'\0' in l or b'\0' in r:continue
   l=l.decode().splitlines(keepends=True);r=r.decode().splitlines(keepends=True)
  except UnicodeDecodeError:continue
  for line in difflib.unified_diff(l,r,fromfile='a/'+name if left is not None else '/dev/null',tofile='b/'+name if right is not None else '/dev/null'):
   patch.append(line if line.endswith('\n') else line+'\n\\ No newline at end of file\n')
 (S/'Preview10-to-Preview11.patch').write_text(''.join(patch))
 (S/'validation/preview10-to-preview11.json').write_text(json.dumps({'baseline':'CityEvolution-Preview-10-Source.zip','files':rows},indent=2))
 (S/'validation/current-changes-from-claude.json').write_text(json.dumps({'baseline':previous.get('baseline','supplied Claude Preview 8'),'files':combined},indent=2))
 # Verify the exact text patch on a text-only Preview 10 checkout.
 check=W/'patch-check';check.mkdir(exist_ok=True)
 for name in rows:
  path=name['path']
  if path not in entries:continue
  raw=z.read(entries[path])
  try:raw.decode()
  except UnicodeDecodeError:continue
  if b'\0' in raw:continue
  p=check/path;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(raw)
subprocess.run(['git','-C',str(check),'apply','--check',str(S/'Preview10-to-Preview11.patch')],check=True)
# Compact audit bundle includes full engine, GUI, tests and both patch stages.
AUD=W/'CityEvolution-Preview-11-Claude-Audit';AUD.mkdir(exist_ok=True)
(AUD/'START-AUDIT-HERE.md').write_text("""# Claude code audit — City Evolution Preview 11
Start with CHANGELOG-Preview-11.md. This is a fictional city-building game fork of LinCity-NG.

The bundle contains full current engine/UI source, tests, exact focused patches, baseline/current hashes, build notes and art mappings/previews. The separate full Source ZIP contains all binary artwork and vendored dependencies.

Review the actual implementation and logs, not only the changelog:
- hideHigh/default reset/restore indicator and isometric occlusion limits;
- 52 sidebar tools, locked-tool browsing, report-only transport, layer selection and speed controls;
- shared 10–30% traffic policy in native and newer vehicle renderers;
- church registration, supplies, research, morale, approval, expenditure, save/load and tests;
- stable map-position variants, 80% sprite scaling over unchanged footprints, smoke offsets and unchanged native frame counts;
- absence of graphical Windows play testing despite successful native simulation/build/import checks.

Changes-from-Claude.patch covers the earlier Preview 8→10 work. Preview10-to-Preview11.patch covers this release. The combined hash index includes binary changes; full binary art is in the complete Source ZIP. The audit ZIP is not a playable game.
""")
for folder in ['src','tests','mk','validation','contrib/preview11','data/gui']:
 shutil.copytree(S/folder,AUD/folder,dirs_exist_ok=True)
for name in ['CMakeLists.txt','CHANGELOG-Preview-11.md','Changes-from-Claude.patch','Preview10-to-Preview11.patch','COPYING.txt']:shutil.copy2(S/name,AUD/name)
for name in ['manifest.json']:
 (AUD/'art').mkdir(exist_ok=True);shutil.copy2(S/'art'/name,AUD/'art'/name)
for p in (S/'art/preview11').glob('*'):
 if p.suffix in ['.json','.jpg']:
  dest=AUD/'art/preview11'/p.name;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest)
print('Text patch verifies; audit bundle ready.',flush=True)
for base,name in [(G,'CityEvolution-Preview-11-Windows-Portable.zip'),(S,'CityEvolution-Preview-11-Source.zip'),(AUD,'CityEvolution-Preview-11-Claude-Audit.zip')]:
 for p in base.rglob('__pycache__'):shutil.rmtree(p)
 paths=sorted(p for p in base.rglob('*') if p.is_file() and p.name!='SHA256SUMS.txt')
 (base/'SHA256SUMS.txt').write_text(''.join(sha(p)+'  '+str(p.relative_to(base))+'\n' for p in paths))
 out=OUT/name
 with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
  for p in sorted(base.rglob('*')):
   if p.is_file():z.write(p,base.name+'/'+str(p.relative_to(base)))
 with zipfile.ZipFile(out) as z:
  assert z.testzip() is None
  for line in (base/'SHA256SUMS.txt').read_text().splitlines():
   digest,rel=line.split('  ',1);assert hashlib.sha256(z.read(base.name+'/'+rel)).hexdigest()==digest
 print(name,out.stat().st_size,sha(out),flush=True)
print('All archives verified',flush=True)
