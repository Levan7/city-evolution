# Art preparation

Run `python contrib/revamp/prepare_assets.py` from any directory to trim/resize masters, rebuild resource bindings and transit thumbnails, and write the model gallery. Requires Python, Pillow, lxml, and DejaVu Sans at the Linux system path used in the script. The source already contains its output. Master images are never modified.

Semantic toolbar icon SVG sources are in data/images/readable. The prior icon generator and SVG renderer are retained for reference; its original art-pack paths must be adapted if regenerating it here.
