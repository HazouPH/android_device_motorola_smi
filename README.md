Motorola SMI
===========================

XT890|Scorpion Mini Intel|Razr i

Motorola Razr i Intel-atom

WIP, don't use unless u know how this works. Still some leftovers from CM11
 |
 V :P and more

working:
- call
- gps
- audio
- All data connections (wifi and cellular)
- Most off the video's
- Houdini 6.1.1a_*

Not working:
- SElinux
- Many more things

What needs to be done:
- Enable fully working SElinux
- Port all specific kernel updates from google's android 3.0 kernel for 5.0+
- Enable camera (api/missing functions)
- Enable A-GPS
- Enable vibrator (port changes to framework or build new vps module)
- Fix WVM codecs
- Fix network search and switch (2g/3g)
- Enable all kinds of optimizations
-- Codecs (VPX)
-- SKIA
-- Renderscript
-- libm
-- ART (Intel extensions and HT)
- Possibly many more

Also debug features are on. Device is a little slow with all those warnings, errors etc.

Source from Turl, Oxavelar, Motorola, Intel and HazouPH

