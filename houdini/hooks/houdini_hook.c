#include <errno.h>
#include <fcntl.h>
#include <cutils/log.h>

/******************************************************************************
 * hook interface for open cpuinfo from ABI2 application
 * java open (libjavacore) and cat(toolbox) open
 *****************************************************************************/
#define APP_WITH_ABI2      "/data/data/.appwithABI2"
#define APP_WITH_ABI2_NEON "/data/data/.appwithABI2neon"
#define CPUINFO            "/proc/cpuinfo"
#define ARM_CPUINFO        "/system/lib/arm/cpuinfo"
#define ARM_CPUINFO_NEON   "/system/lib/arm/cpuinfo.neon"

int houdini_hook_open_neon(int myuid, int flags, int mode) {
    int fd = -1;

    int pkg_neon_ABI2 = TEMP_FAILURE_RETRY(open(APP_WITH_ABI2_NEON, O_RDONLY, 0444));
    if (pkg_neon_ABI2 != -1) {
        int pkguid = 0;
        while (TEMP_FAILURE_RETRY(read(pkg_neon_ABI2, &pkguid, 4) > 0)) {
            if (myuid == pkguid) {
                fd = TEMP_FAILURE_RETRY(open(ARM_CPUINFO_NEON, flags, mode));
                ALOGD("Neon Apps with ABI2 %d accessing /proc/cpuinfo \n", fd);
                break;
            }
        }
        TEMP_FAILURE_RETRY(close(pkg_neon_ABI2));
    }

    return fd;
}

int houdini_hook_open(const char *path, int flags, int mode) {
    int fd = -1;

    if (strncmp(path, CPUINFO, sizeof(CPUINFO)) == 0) {
        int myuid = getuid();

        int pkg_ABI2 = TEMP_FAILURE_RETRY(open(APP_WITH_ABI2, O_RDONLY,0444));
        if (pkg_ABI2 != -1) {
            int pkguid = 0;
            ALOGD("Searching package installed with ABI2 with Uid: %d \n", myuid);

            while (TEMP_FAILURE_RETRY(read(pkg_ABI2, &pkguid, 4) > 0)) {
                if (myuid == pkguid) {
                    fd = houdini_hook_open_neon(myuid, flags, mode);
                    if (fd == -1) {
                        fd = TEMP_FAILURE_RETRY(open(ARM_CPUINFO, flags, mode));
                        ALOGD("Apps with ABI2 %d accessing /proc/cpuinfo \n", fd);
                    }
                    break;
                }
            }

            TEMP_FAILURE_RETRY(close(pkg_ABI2));
        }
    }

    if (fd == -1) {
        fd = TEMP_FAILURE_RETRY(open(path, flags, mode));
    }

    return fd;
}
