/*
 * Copyright 2017 The Android Open Source Project
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

#pragma once

#ifndef LOG_TAG
#warning "GrallocLoader.h included without LOG_TAG"
#endif

#include <memory>

#include <allocator-hal/2.0/Allocator.h>
#include "Gralloc0Hal.h"
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <log/log.h>

namespace android {
namespace hardware {
namespace graphics {
namespace allocator {
namespace V2_0 {
namespace passthrough {

class GrallocLoader {
   public:
    static IAllocator* load() {
        const hw_module_t* module = loadModule();
        if (!module) {
            return nullptr;
        }
        auto hal = createHal(module);
        if (!hal) {
            return nullptr;
        }
        return createAllocator(std::move(hal));
    }

    // load the gralloc module
    static const hw_module_t* loadModule() {
        const hw_module_t* module;
        int error = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module);
        if (error) {
            ALOGE("failed to get gralloc module");
            return nullptr;
        }

        return module;
    }

    // create an AllocatorHal instance
    static std::unique_ptr<hal::AllocatorHal> createHal(const hw_module_t* module) {
        auto hal = std::make_unique<Gralloc0Hal>();
        return hal->initWithModule(module) ? std::move(hal) : nullptr;
    }

    // create an IAllocator instance
    static IAllocator* createAllocator(std::unique_ptr<hal::AllocatorHal> hal) {
        auto allocator = std::make_unique<hal::Allocator>();
        return allocator->init(std::move(hal)) ? allocator.release() : nullptr;
    }
};

}  // namespace passthrough
}  // namespace V2_0
}  // namespace allocator
}  // namespace graphics
}  // namespace hardware
}  // namespace android
