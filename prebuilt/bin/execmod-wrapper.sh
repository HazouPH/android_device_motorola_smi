#!/system/bin/sh

if [ "${0}" = "/system/bin/akmd8963.sh" ]; then
	exec "/system/bin/akmd8963" "${@}"
elif [ "${0}" = "/system/bin/batt_health.sh" ]; then
	exec "/system/bin/batt_health" "${@}"
elif [ "${0}" = "/system/bin/bd_prov.sh" ]; then
	exec "/system/bin/bd_prov" "${@}"
elif [ "${0}" = "/system/bin/pvrsrvctl.sh" ]; then
	exec "/system/bin/pvrsrvctl" "${@}"
elif [ "${0}" = "/system/bin/gps_driver.sh" ]; then
	exec "/system/bin/gps_driver" "${@}"
elif [ "${0}" = "/system/bin/mmgr.sh" ]; then
	exec "/system/bin/mmgr" "${@}"
fi

exit 1
