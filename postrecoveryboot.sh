#!/sbin/sh
/sbin/watchdogd &

# Lower brightness
echo 50 > /sys/class/backlight/psb-bl/brightness

# CPU stability and less heat
echo interactive > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
echo 1400000 > /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq
echo 1400000 > /sys/devices/system/cpu/cpu1/cpufreq/scaling_max_freq
