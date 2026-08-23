# Copyright and modification notice

Copyright (C) 2026 CompileCraftWorks and Fitting Schlongs contributors.

Fitting Schlongs contains a separately packaged and modified version of the
SOS/TNG slot 32/49 correction originally developed within **Skyrim Fitting
System**, a GNU GPL version 3 project.

The standalone work was created and subsequently modified in 2026 to:

- run independently as `FittingSchlongs.dll` without an ESP;
- disable itself when `SkyrimFittingSystem.dll` is present;
- classify modular slot 32 upper-only and slot 49 lower-body equipment;
- give the classifier's result priority over conflicting SOS/TNG or KID-added
  keywords;
- preserve default SOS/TNG behavior for ordinary slot 32 full-body armor;
- exclude unrelated slot 52 equipment from genital-armor filtering; and
- support Skyrim SE and AE through CommonLibSSE-NG and Address Library IDs.

This notice records that the work was modified in 2026. The full GPLv3 license
is in `LICENSE`.

Names and trademarks belonging to Bethesda Game Studios, SKSE, Schlongs of
Skyrim, and The New Gentleman are used only to describe compatibility.
