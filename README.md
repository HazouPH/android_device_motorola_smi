Motorola SMI
===========================

XT890|Scorpion Mini Intel|Razr i

Motorola Razr i Intel-atom

Source from Turl, Oxavelar, Motorola, Intel and HazouPH

---------------------------------------------------------

To build CM11 from with this device tree:
- Setup an Android 4.4.4 (CM11) build environment
- Repo init CM11 in your build directory
- Download/Copy over 'local_manifest.xml to:
(build-directory)/.repo/local_manifests/local_manifest.xml
- Execute "Repo Sync" in your build directory
- Execute the command from repopick.txt in the right directory (repo)
- Now do all the commands to get a build starting, 
'extract-files.sh is not needed because of the vendor map,
the build commands will trigger the patching of the source
- Use "breakfast" and "make/mka (bacon)" commands to start the build!

