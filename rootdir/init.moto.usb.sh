#!/system/bin/sh

usb_config=`getprop sys.usb.config`

bootmode_usb_config ()
{
    # ro.bootmode has the powerup reason
    bootmode=`getprop ro.bootmode`
    persist_usb_config=`getprop persist.sys.usb.config`
    echo "moto-usb-sh: bootmode = \"$bootmode\""

    # Use ro.serialno if available
    serialno=`getprop ro.serialno`
    case "$serialno" in
        "")
        ;;
        *)
            echo "moto-usb-sh: using serialno = \"$serialno\""
            echo -n "$serialno" > /sys/class/android_usb/android0/iSerial
        ;;
    esac

    case "$bootmode" in
        "bp-tools" )
            setprop persist.sys.usb.config modem,usbnet,adb
            # USB tethering modes
            setprop ro.usb.rndis.pid 0x7109
            setprop ro.usb.rndis.config rndis,usbnet
            setprop ro.usb.rndis-adb.pid 0x710A
            setprop ro.usb.rndis-adb.config rndis,usbnet,adb
        ;;
        "factory" )
            allow_adb=`getprop persist.factory.allow_adb`
            case "$allow_adb" in
                "1")
                    setprop persist.sys.usb.config usbnet,adb
                ;;
                *)
                    setprop persist.sys.usb.config usbnet
                ;;
            esac
            # USB tethering modes
            setprop ro.usb.rndis.pid 0x710B
            setprop ro.usb.rndis.config rndis
            setprop ro.usb.rndis-adb.pid 0x710C
            setprop ro.usb.rndis-adb.config rndis,adb
        ;;
        * )
            case "$persist_usb_config" in
                "ptp,adb" | "mtp,adb" | "mass_storage,adb" | "ptp" | "mtp" | "mass_storage" )
                ;;
                *)
                    buildtype=`getprop ro.build.type`
                    case "$buildtype" in
                        "user" )
                            setprop persist.sys.usb.config mtp
                        ;;
                        * )
                            setprop persist.sys.usb.config mtp,adb
                        ;;
                    esac
                ;;
            esac
            # USB tethering modes
            setprop ro.usb.rndis.pid 0x710B
            setprop ro.usb.rndis.config rndis
            setprop ro.usb.rndis-adb.pid 0x710C
            setprop ro.usb.rndis-adb.config rndis,adb
        ;;
    esac
}


tcmd_ctrl_adb ()
{
    ctrl_adb=`getprop tcmd.ctrl_adb`
    echo "moto-usb-sh: tcmd.ctrl_adb = $ctrl_adb"
    case "$ctrl_adb" in
        "0")
            if [[ "$usb_config" == *adb* ]]
            then
                # *** ALWAYS expecting adb at the end ***
                new_usb_config=${usb_config/,adb/}
                echo "moto-usb-sh: disabling adb ($new_usb_config)"
                setprop sys.usb.config $new_usb_config
                setprop persist.factory.allow_adb 0
            fi
        ;;
        "1")
            if [[ "$usb_config" != *adb* ]]
            then
                # *** ALWAYS expecting adb at the end ***
                new_usb_config="$usb_config,adb"
                echo "moto-usb-sh: enabling adb ($new_usb_config)"
                setprop sys.usb.config $new_usb_config
                setprop persist.factory.allow_adb 1
            fi
        ;;
    esac

}


tcmd_ctrl_at_tunnel ()
{
    ctrl_at_tunnel=`getprop tcmd.ctrl_at_tunnel`
    echo "moto-usb-sh: tcmd.ctrl_at_tunnel = $ctrl_at_tunnel"

    case "$ctrl_at_tunnel" in
        "0")
            if [[ "$usb_config" == *acm* ]]
            then
                # Remove acm from sys.usb.config
                new_usb_config=${usb_config/acm,/}
                echo "moto-usb-sh: disabling AT Tunnel ($new_usb_config)"
                stop proxy
                stop proxy-tunneling
                setprop sys.usb.config $new_usb_config
            fi
        ;;
        "1")
            if [[ "$usb_config" != *acm* ]]
            then

                # add acm to sys.usb.config
                new_usb_config="acm,$usb_config"
                echo "moto-usb-sh: enabling AT Tunnel ($new_usb_config)"
                stop proxy
                setprop sys.usb.config $new_usb_config
                start proxy-tunneling
            fi
        ;;
    esac
}


usb_action=`getprop usb.moto-usb-sh.action`
echo "moto-usb-sh: action = \"$usb_action\""

case "$usb_action" in
    "")
        bootmode_usb_config
    ;;
    "tcmd.ctrl_adb")
        tcmd_ctrl_adb
    ;;
    "tcmd.ctrl_at_tunnel")
        case "$usb_config" in
            "usbnet,adb" | "usbnet" | "acm,usbnet,adb" | "acm,usbnet")
            tcmd_ctrl_at_tunnel
            ;;
        esac
    ;;
    *)
    ;;
esac
