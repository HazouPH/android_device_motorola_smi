/*
 **
 ** Copyright 2012 Intel Corporation
 **
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 **
 **      http://www.apache.org/licenses/LICENSE-2.0
 **
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 */
#define LOG_TAG "BlueToothAudio"

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include <utils/Log.h>



#include "BluetoothAudio.h"


namespace android {


CBluetoothAudio::CBluetoothAudio()
{
}

// Enable
bool CBluetoothAudio::enablePort(bool bEnabled)
{
    int pid;

    const char logwrapper_path[] = "/system/bin/logwrapper";

    if ((pid = fork()) != 0) {
        // Father
        if (pid == -1) {
            ALOGE("%s - Cannot fork() to call hcitool: %s", __FUNCTION__, strerror(errno));
            return false;
        }

        waitpid(pid, NULL, 0);

    } else {
        // Child
        if (bEnabled) {
            const char * argv[] = {"logwrapper", "/system/xbin/hcitool", "cmd", "0x3F", "0X195", "FF", "FF", "FF",
                                   "FF", "FF", "FF", "FF", "FF", "01", "FF", "FF", "00",
                                   "00", "00", "00", NULL};
            ALOGI("%s : enabling BT path", __FUNCTION__);

            execv(logwrapper_path, (char **)argv);

        } else {
            const char * argv[] = {"logwrapper", "/system/xbin/hcitool", "cmd", "0x3F", "0X195", "FF", "FF", "FF",
                                   "FF", "FF", "FF", "FF", "FF", "00", "FF", "FF", "00",
                                   "00", "00", "00", NULL};
            ALOGI("%s : Disabling BT path", __FUNCTION__);

            execv(logwrapper_path, (char **)argv);

        }
        ALOGE("%s - Failed to exec hcitool command for %s: %s", __FUNCTION__, bEnabled ? "enabling" :"disabling", strerror(errno));
    }

    return true;
}

}; // namespace android
