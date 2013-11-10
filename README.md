Motorola SMI
===========================

Motorola Razr i Intel-atom

Repository is not yet a fully working tree. There are still some parts missing or not working.

'SMI-Plus' is only needed for rom building! (to free up space)

Source from Turl, Oxavelar, motorola and HazouPH

Still to do for fully working tree:
- Ramdisk build working correctly (using prebuilt for now)
- More...

(Only for CM10.1!!! nov 2013 repo). 

- Setup building enviroment for CM10 (needs 64 bits OS)
- Git init my android manifest: repo init -u git://github.com/HazouPH/android.git -b cm-10.1
- Repo sync
- Apply all patches by executing ApplyPatches.sh in device/motorola/smi/patch
- After every new 'Repo sync' u need to repatch the repository with ApplyPatches.sh.

Good luck!
