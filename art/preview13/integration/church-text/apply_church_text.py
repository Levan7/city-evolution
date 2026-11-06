#!/usr/bin/env python3
"""Change Church display wording only. Dry run unless --apply is supplied."""
from pathlib import Path
import argparse,json,shutil

ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('project',type=Path,help='current project root containing src/ and data/')
ap.add_argument('--apply',action='store_true')
a=ap.parse_args();root=a.project.resolve();here=Path(__file__).resolve().parent
spec=json.loads((here/'text-changes.json').read_text());pending={};errors=[]
for item in spec['files']:
    p=root/item['path']
    if not p.exists():
        if item.get('required'):errors.append('Missing '+item['path'])
        else:print('Optional file absent:',item['path'])
        continue
    before=p.read_text();after=before
    for edit in item['replacements']:
        if edit['old'] in after:after=after.replace(edit['old'],edit['new'])
        elif edit['new'] not in after:
            if item.get('required'):errors.append('Text changed in '+item['path']+'; merge church-name.patch manually.')
            else:print('Optional wording changed; review manually:',item['path'])
    if after!=before:pending[p]=after
if errors:raise SystemExit('No files written.\n'+'\n'.join(errors))
print(str(len(pending))+' text files would change. No image, model, resource ID or gameplay edit.')
if not a.apply:raise SystemExit(0)
for p,text in pending.items():
    backup=root/'church-name-backup'/p.relative_to(root)
    if not backup.exists():backup.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,backup)
    p.write_text(text)
print('Church wording applied. Existing artwork and behavior retained.')
