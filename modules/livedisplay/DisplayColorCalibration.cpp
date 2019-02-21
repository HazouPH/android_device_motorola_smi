/*
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

#define LOG_TAG "vendor.lineage.livedisplay@2.0-service-smi"

#include <stdio.h>
#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>

#include <vendor/lineage/livedisplay/2.0/IDisplayColorCalibration.h>

using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;

#define COLOR_FILE "/sys/devices/platform/kcal_ctrl.0/kcal"
#define KCAL_MIN_FILE "/sys/devices/platform/kcal_ctrl.0/kcal_min"

#define MIN 0;
#define MAX 255;

class DisplayColorCalibration : public vendor::lineage::livedisplay::V2_0::IDisplayColorCalibration {
    
    private:
    FILE *color_file;

    public:
    DisplayColorCalibration(void) {
        color_file = fopen(COLOR_FILE, "r+");
        if(!color_file) {
            LOG(ERROR) << "Could not open " << COLOR_FILE
                       << " error: " << strerror(errno);
        }
    }

    Return<int32_t> getMaxValue() override {
        return MAX;
    }

    Return<int32_t> getMinValue() override {
        FILE *min_file;
        int min_val;
        
        min_file = fopen("KCAL_MIN_FILE", "r");
        if(min_file) {
            fscanf(min_file, "%d", &min_val);
        }
        else {
            min_val = MIN;
        }
        return min_val;
    }

    Return<void> getCalibration(getCalibration_cb _hidl_cb) override {
        std::vector<int32_t> rgb(3);

        if(color_file){
            fscanf(color_file, "%d %d %d", &rgb[0], &rgb[1], &rgb[2]);
            rewind(color_file);
        }

        _hidl_cb(rgb);
        return Void();
    }

    Return<bool> setCalibration(const hidl_vec<int32_t>& rgb) override {
        if(color_file) {
            fprintf(color_file, "%d %d %d", rgb[0], rgb[1], rgb[2]);
            rewind(color_file);
            return true;
        }
        else {
            return false;
        }
    }
};

int main() {
    android::sp<DisplayColorCalibration> dcc;
    android::status_t status;

    LOG(INFO) << "LiveDisplay HAL service is starting.";

    dcc = new DisplayColorCalibration();
    android::hardware::configureRpcThreadpool(1, true /*callerWillJoin*/);
    status = dcc->registerAsService();
    if (status != android::OK) {
        LOG(ERROR) << "Could not register service for LiveDisplay HAL DisplayColorCalibration"
                   << " Iface (" << status << ")";
        goto shutdown;
    }
    android::hardware::joinRpcThreadpool();

shutdown:
    LOG(ERROR) << "LiveDisplay HAL service is shutting down.";
    return 1;
}
