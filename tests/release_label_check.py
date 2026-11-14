#!/usr/bin/env python3
"""Checks that every visible release label agrees with src/lincity-ng/release_label.hpp."""
import re, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
errors = []
hpp = (root / 'src/lincity-ng/release_label.hpp').read_text()
number = int(re.search(r'CE_RELEASE_NUMBER\s+(\d+)', hpp).group(1))
label = re.search(r'CE_RELEASE_LABEL\s+"([^"]+)"', hpp).group(1)
if not re.fullmatch(rf'Preview {number}(?: Hotfix [1-9]\d*)?', label):
    errors.append(f'label "{label}" does not match number {number}')
main = (root / 'src/lincity-ng/main.cpp').read_text()
if 'CE_RELEASE_LABEL' not in main: errors.append('main.cpp window title does not use CE_RELEASE_LABEL')
if re.search(r'"City Evolution - Preview \d+', main): errors.append('main.cpp still has a hard-coded release label')
menu = (root / 'data/gui/mainmenu.xml').read_text()
found = re.findall(r'Preview \d+(?: Hotfix \d+)?', menu)
if found != [label]: errors.append(f'main menu shows {found}, expected [{label!r}]')
rc = (root / 'contrib/windows/launcher.rc').read_text()
if f'FILEVERSION 0,{number},0,0' not in rc or f'PRODUCTVERSION 0,{number},0,0' not in rc: errors.append('launcher.rc numeric version is out of date')
for key in ('FileVersion', 'ProductVersion'):
    if f'VALUE "{key}", "{label}' not in rc: errors.append(f'launcher.rc {key} is not "{label}"')
changelog = (root / 'CHANGELOG.md').read_text()
if f'## City Evolution — Preview {number}\n' not in changelog:
    errors.append(f'Preview {number} is missing from CHANGELOG.md')
if ' Hotfix ' in label:
    section = changelog.partition(f'## City Evolution — Preview {number}\n')[2]
    section = section.split('\n## ', 1)[0]
    hotfix = label.removeprefix(f'Preview {number} ')
    if f'### {hotfix}' not in section.splitlines():
        errors.append(f'{label} is missing from CHANGELOG.md')
if errors:
    print('FAIL: ' + '; '.join(errors)); sys.exit(1)
print(f'PASS: release label "{label}" is consistent (window title, main menu, Windows version resource, changelog)')
