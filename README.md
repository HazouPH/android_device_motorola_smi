Motorola SMI
===========================

Motorola Razr i Intel-atom

Repository is not yet a fully working tree. There are still some parts missing.

What will build and work:
Build for recovery and AOSP

What doenst work, but builds:
The build for CM doenst work ATM. CM has an ARM based source.

AOSP specific patches. In the "patch" folder are specific AOSP4.2.2 patches. execute "applypatches.sh" always after a new "repo sync" and before a build.

Source from Turl, STS-dev-team, motorola and HazouPH

Still to do for fully working tree:
- Make smi Hall modules (Dont have all the sources)
- Ramdisk build working
- Add CM10 patches for building
