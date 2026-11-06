# Visual interface update 0.4

The Windows executable is the user-confirmed working UCRT 0.3.1 binary. This is a data-driven interface/artwork update, without rebuilding or changing that executable.

New material: start-menu layout and generated city background; native construction-toolbar Evolution category with group IDs 51-54; four building sprite icons; a new SVG outline icon family with per-tool badges; redesigned speed/system controls; navy new-world/settings/load/save screens; themed City hall; visible full-width City hall control. A copy of the supplied showcase is provided as the initial Continue city so new artwork appears on first use.

World sprites remain mostly the original art. The four generated building sprites from 0.3 remain; no claim is made of a completed terrain/building-art overhaul, municipal AI, airports or expanded public transport.

Background asset: data/images/evolution/city-menu.png. Built-in image-generation prompt: modern isometric city at late-afternoon/evening light, research campus, mixed-use neighborhoods, freight railway and warehouse beside river, wind turbines and roads; city on right two-thirds, quiet navy left third for readable native menu; no text/logo/UI.

Icons and button plates are original code-native SVG artwork, rasterized with librsvg/Cairo for the engine. SVG sources are included beside PNGs. Building icons are scaled asset derivatives of the four included building sprites.

Validation: XML parsing, XSL transformation, control-name bindings, menu/group references, asset existence and unchanged binary hashes. Windows rendered appearance/click interactions have not been verified here.
