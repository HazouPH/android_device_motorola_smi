Motorola SMI
===========================

Motorola Razr i Intel-atom

Repository is not yet a fully working tree. There are still some parts missing or not working.

'SMI-Plus' is only needed for rom building! (to free up space)

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
- Repo sync
- Git clone https://github.com/hazouph/android_device_motorola_smi to device/motorola/smi
- Add/copy the device/motorola/smi/patch/local_manifest.xml to (source)/.repo/local_manifests/local_manifest.xml
- Synchronise your repository (repo sync) again
- Apply all patches by executing ApplyPatches.sh
- After every 'Repo sync' u need to repatch the repository.
