Motorola SMI
===========================

XT890|Scorpion Mini Intel|Razr i

Motorola Razr i Intel-atom

Source from Turl, Oxavelar, Motorola, Intel and HazouPH

---------------------------------------------------------

To build LineageOS: cm-11.0, from with this device tree:
- Setup an Android 4.4.4 (cm-11.0) build environment
- Repo init cm-11.0 in your build directory
- Download/Copy over 'local_manifest.xml to:
(build-directory)/.repo/local_manifests/local_manifest.xml
- Execute "Repo Sync" in your build directory
- Execute the command from repopick.txt in the right directory (repo), if exist
- Look at the patches in smi-patches if there are any
- Now do all the commands to get a build starting, 
'extract-files.sh is not needed because of the vendor map
- Use "breakfast" and "make/mka (bacon)" commands to start the build!

