#!/usr/bin/env python3
"""Check style availability in GUI import order, without SDL or a game window."""
import argparse
import re
from pathlib import Path
import xml.etree.ElementTree as ET


def audit(data, nls):
    errors, visited = set(), set()

    def walk_file(relative, styles, stack=()):
        path = data / relative
        if relative in stack:
            errors.add(f"{relative}: cyclic GUI import")
            return
        visited.add(relative)
        # The build runs every GUI file through an xsl filter (identity.xsl or dialogs.xsl); that needs this line.
        if not re.search(r'<\?xml-stylesheet\s+href="[^"]+\.xsl"', path.read_text(encoding='utf-8', errors='ignore')):
            errors.add(f'{relative}: missing the xml-stylesheet line (the build cannot process this file)')
        root = ET.parse(path).getroot()

        def walk(node):
            tag = node.tag.rsplit('}', 1)[-1]
            if tag == 'if-NLS' and node.get('value') != nls:
                return
            style = node.get('style')
            if style and style not in styles:
                errors.add(f'{relative}: no style "{style}"')
            if tag == 'DefineStyle':
                styles.add(node.attrib['name'])
            target = node.get('src', '')
            if tag in ('Import', 'Document') and target.startswith('gui/') and target.endswith('.xml'):
                walk_file(target, styles, (*stack, relative))
            for child in node:
                walk(child)

        walk(root)

    # MainMenu loads all its pages before Game loads app.xml. The registry persists.
    startup_styles = set()
    walk_file('gui/mainmenu.xml', startup_styles)
    walk_file('gui/app.xml', startup_styles)
    # Dialogs and other independently loaded screens inherit that registry.
    for path in sorted((data / 'gui').rglob('*.xml')):
        relative = path.relative_to(data).as_posix()
        if relative not in visited:
            walk_file(relative, startup_styles.copy())
    return sorted(errors), len(visited)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, default=Path(__file__).resolve().parents[1] / 'data')
    args = parser.parse_args()
    failed = False
    for nls in ('enabled', 'disabled'):
        errors, count = audit(args.data_root, nls)
        for error in errors:
            print(f'FAIL ({nls}): {error}')
        if errors:
            failed = True
        else:
            print(f'PASS: {count} GUI XML files; styles defined before use (NLS {nls})')
    return int(failed)


if __name__ == '__main__':
    raise SystemExit(main())
