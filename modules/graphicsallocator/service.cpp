/*
 * Copyright 2016 The Android Open Source Project
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

#define LOG_TAG "android.hardware.graphics.allocator@2.0-service.smi"

#include "GrallocLoader.h"
#include <android/hardware/graphics/allocator/2.0/IAllocator.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>

using android::hardware::graphics::allocator::V2_0::IAllocator;
using android::hardware::graphics::allocator::V2_0::passthrough::GrallocLoader;
using ::android::hardware::configureRpcThreadpool;
using ::android::hardware::joinRpcThreadpool;
using ::android::sp;

int main() {
    sp<IAllocator> allocator = GrallocLoader::load();
    configureRpcThreadpool(4, true);
    android::status_t status = allocator->registerAsService();
    if (status == android::OK)
        joinRpcThreadpool();
    else
        ALOGE("Could not register as a service!");
}
