# RazrIChargeCurrent
Application to manage the sysfs settings of the intel medfield battery charging circuit as used in the Motorola Razr I

# What does it do?
## Default kernel

This application allows you to limit the charging speed of your device to 500 mA.

## Patched kernel

When you apply the patch to your intel_medfield_battery from http://forum.xda-developers.com/showthread.php?t=2667650 or https://gist.github.com/NerdyProjects/91f466bfc98921945726 and rebuild your kernel module, you can:

- limit the usb input current to 500 mA (as above)
- force the usb input current limit up to 950 mA
- force the usb input current to hardware maximum

BEWARE: This may brick your power supply (USB port at computer, laptop, whatever).

# Additional licenses
The application logo is done by http://thesquid.ink/ and licensed under CC BY 3.0 (http://creativecommons.org/licenses/by/3.0/).

# Author
NerdyProjects / PosixCompatible
https://github.com/NerdyProjects
