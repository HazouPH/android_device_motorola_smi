Motorola SMI
===========================

Motorola Razr i Intel-atom

Repository is not yet a fully working tree. There are still some parts missing.

What will build and work:
- Recovery

What doenst work, but builds and boot(loops):
- CM
- AOSP

Source from Turl, Oxavelar, motorola and HazouPH

Still to do for fully working tree:
- Ramdisk build working correctly

To use ths repository for building recovery's (Only on CM10.1!!! September 2013 repo). 

- Git init the CM10.1 repository
- Add/copy the patch/local_manifest.xml to (source)/.repo/local_manifests/localmanifest.xml
- Delete one of the 2 following lines in local_manifest.xml
- - For TWRP: remove-project name="CyanogenMod/android_bootable_recovery" /
- - For CWM: remove-project name="Team-Win-Recovery-Project" /
- Synchronise your repository
- Apply all patches with the ApplyPatches.sh
- After every 'Repo sync' u need to repatch the repository.
