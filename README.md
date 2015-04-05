Motorola SMI
===========================

XT890|Scorpion Mini Intel|Razr i

Motorola Razr i Intel-atom

Source from Turl, Oxavelar, Motorola, Intel and HazouPH

Still todo:
- Kernel build when source is available, takes long

---------------------------------------------------------

To build CM11 from with this device tree:
- Setup an Android 4.4.4 (CM11) build environment
- Repo init CM11 in your build directory
- Download/Copy over 'local_manifest.xml to:
(build-directory)/.repo/local_manifests/local_manifest.xml
- Execute "Repo Sync" in your build directory
- Execute 'sync_and_patch.sh' in the map /patches
- Now do all the commands to get a build starting, 
'extract-files.sh is not needed because of the vendor map,
Use lunch and 'make/mka' to start the build!

