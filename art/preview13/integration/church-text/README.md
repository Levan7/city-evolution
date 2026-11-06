# Church — text update

The public building name is **Church**; plural **Churches**. There is no
geographic or denominational identification in the building name or Help text.

The model, church artwork, animations, costs, upkeep and gameplay benefits
remain exactly as before. Existing save/resource identifiers are retained for
compatibility and are not displayed as the building's public name.

The update changes the building's singular/plural display strings, Help title,
and corresponding descriptions in the project README and changelog. The Help
body already describes only costs, supplies and benefits, so it is preserved.

`church-name.patch` shows the exact edits against Preview 11 source version 12.
For the current rewrite, merge those small text edits rather than copying an
older source tree over it. `text-changes.json` and `apply_church_text.py` apply
only the listed wording changes and preserve unrelated edits and Help images.

```bash
python apply_church_text.py /path/to/current/source
python apply_church_text.py /path/to/current/source --apply
```

The first command is a dry run. The second backs up and updates affected text
files. Rebuild the game after changing its compiled building label. The small
patch itself is not a replacement executable. Merge it after the art pack in
the current rewrite; no image assets need changing.
