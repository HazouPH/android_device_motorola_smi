/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2019 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/hardware/light/2.0/ILight.h>
#include <hidl/HidlTransportSupport.h>

#include <android-base/logging.h>

#include <stdlib.h>

#define BACKLIGHT_FILE "/sys/class/backlight/psb-bl/brightness"
#define LED_FILE "/sys/class/leds/rgb/control"

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {
struct Light : public ILight {
  private:
    FILE *fd_backlight;
    FILE *fd_led;
    LightState mAttentionState;
    LightState mNotificationState;
    LightState mBatteryState;
    
  public:
    Light() {
        fd_backlight = 0;
        fd_led = 0;
        mAttentionState.color = 0xff000000;
        mNotificationState.color = 0xff000000;
        mBatteryState.color = 0xff000000;
    }

    ~Light() {
        if(fd_backlight)
            fclose(fd_backlight);
        if(fd_led)
            fclose(fd_led);
    }

    Return<void> getSupportedTypes(getSupportedTypes_cb _hidl_cb) override {
        std::vector<Type> types = {
            Type::BACKLIGHT,
            Type::ATTENTION,
            Type::NOTIFICATIONS,
            Type::BATTERY
        };

        _hidl_cb(types);
        return Void();
    }

    Return<Status> setLight(Type type, const LightState& state) override {
        Status ret = Status::SUCCESS;
        int need_led_update = 0;

        switch(type) {
            case Type::BACKLIGHT:
                if(fd_backlight || (fd_backlight = fopen(BACKLIGHT_FILE, "w"))) {
                    fprintf(fd_backlight, "%i", (state.color & 0xff) * 100 / 0xff);
                    rewind(fd_backlight);
                }
                else {
                    LOG(ERROR) << "Failed to open " << BACKLIGHT_FILE << ", error="
                       << errno << " (" << strerror(errno) << ")";
                    ret = Status::UNKNOWN;
               }
                break;
            case Type::ATTENTION:
                mAttentionState = state;
                need_led_update = 1;
                break;
            case Type::NOTIFICATIONS:
                mNotificationState = state;
                need_led_update = 1;
                break;
            case Type::BATTERY:
                mBatteryState = state;
                need_led_update = 1;
                break;
            default:
                ret = Status::LIGHT_NOT_SUPPORTED;
        }
        if(need_led_update) {
            LightState* current_state;
            if (mAttentionState.color & 0x00ffffff)
                current_state = &mAttentionState;
            else if (mNotificationState.color & 0x00ffffff)
                current_state = &mNotificationState;
            else
                current_state = &mBatteryState;
            if(fd_led || (fd_led = fopen(LED_FILE, "w"))) {
                fprintf(fd_led, "%06X %d %d %d %d", current_state->color, current_state->flashOnMs, current_state->flashOffMs, 50, 50);
                rewind(fd_led);
            }
            else {
                LOG(ERROR) << "Failed to open " << LED_FILE << ", error=" << errno
                   << " (" << strerror(errno) << ")";
                ret = Status::UNKNOWN;
            }
        }
        return ret;
    }
};
}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android

using android::hardware::light::V2_0::ILight;
using android::hardware::light::V2_0::implementation::Light;

int main(int /* argc */, char* /* argv */ []) {
    android::hardware::configureRpcThreadpool(1 /*threads*/, true /*willJoin*/);

    android::sp<ILight> light = new Light();
    const android::status_t status = light->registerAsService();
    if (status != ::android::OK) {
        return 1;
    }
    android::hardware::joinRpcThreadpool();
    return 1; // joinRpcThreadpool should never return
}
