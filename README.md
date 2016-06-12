Motorola SMI
===========================

XT890|Scorpion Mini Intel|Razr i

Motorola Razr i Intel-atom

WIP, don't use unless u know how this works. Still some leftovers from CM11
 |
 V :P and more

Source from Turl, Oxavelar, Motorola, Intel and HazouPH

---------------------------------------------------------

To build CM11 from with this device tree:
- Setup an Android 4.4.4 (CM11) build environment
- Repo init CM11 in your build directory
- Download/Copy over 'local_manifest.xml to:
(build-directory)/.repo/local_manifests/local_manifest.xml
- Execute "Repo Sync" in your build directory
- Now do all the commands to get a build starting, 
'extract-files.sh is not needed because of the vendor map,
the build commands will trigger the patching of the source
- Don't forget to download the prebuilts in vendor/cm
- Use "breakfast" and "make/mka" commands to start the build!

