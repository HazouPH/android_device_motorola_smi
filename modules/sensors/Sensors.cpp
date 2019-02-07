/*
 * Copyright (C) 2016 The Android Open Source Project
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

#include "Sensors.h"

#include <android-base/logging.h>
#include <sensors/convert.h>

#include <sys/stat.h>

namespace android {
namespace hardware {
namespace sensors {
namespace V1_0 {
namespace implementation {

static Result ResultFromStatus(status_t err) {
    switch (err) {
        case OK:
            return Result::OK;
        case PERMISSION_DENIED:
            return Result::PERMISSION_DENIED;
        case NO_MEMORY:
            return Result::NO_MEMORY;
        case BAD_VALUE:
            return Result::BAD_VALUE;
        default:
            return Result::INVALID_OPERATION;
    }
}

Sensors::Sensors()
    : mInitCheck(NO_INIT),
      mSensorModule(nullptr),
      mSensorDevice(nullptr) {
    status_t err = OK;
    err = hw_get_module( SENSORS_HARDWARE_MODULE_ID,
            (hw_module_t const **)&mSensorModule);
    if (mSensorModule == NULL) {
        err = UNKNOWN_ERROR;
    }

    if (err != OK) {
        LOG(ERROR) << "Couldn't load "
                   << SENSORS_HARDWARE_MODULE_ID
                   << " module ("
                   << strerror(-err)
                   << ")";

        mInitCheck = err;
        return;
    }

    err = sensors_open(&mSensorModule->common, &mSensorDevice);

    if (err != OK) {
        LOG(ERROR) << "Couldn't open device for module "
                   << SENSORS_HARDWARE_MODULE_ID
                   << " ("
                   << strerror(-err)
                   << ")";

        mInitCheck = err;
        return;
    }

    mInitCheck = OK;
}

status_t Sensors::initCheck() const {
    return mInitCheck;
}

Return<void> Sensors::getSensorsList(getSensorsList_cb _hidl_cb) {
    sensor_t const *list;
    size_t count = mSensorModule->get_sensors_list(mSensorModule, &list);

    hidl_vec<SensorInfo> out;
    out.resize(count);

    for (size_t i = 0; i < count; ++i) {
        const sensor_t *src = &list[i];
        SensorInfo *dst = &out[i];

        convertFromSensor(*src, dst);
        if(src->type == SENSOR_TYPE_PROXIMITY) {
            dst->flags = SENSOR_FLAG_WAKE_UP | SENSOR_FLAG_ON_CHANGE_MODE;
        }
    }

    _hidl_cb(out);

    return Void();
}

Return<Result> Sensors::setOperationMode(OperationMode mode) {
    return Result::INVALID_OPERATION;
}

Return<Result> Sensors::activate(
        int32_t sensor_handle, bool enabled) {
    return ResultFromStatus(
            mSensorDevice->activate(
                mSensorDevice,
                sensor_handle,
                enabled));
}

Return<void> Sensors::poll(int32_t maxCount, poll_cb _hidl_cb) {

    hidl_vec<Event> out;
    hidl_vec<SensorInfo> dynamicSensorsAdded;

    std::unique_ptr<sensors_event_t[]> data;
    int err = android::NO_ERROR;

    { // scope of reentry lock

        // This enforces a single client, meaning that a maximum of one client can call poll().
        // If this function is re-entred, it means that we are stuck in a state that may prevent
        // the system from proceeding normally.
        //
        // Exit and let the system restart the sensor-hal-implementation hidl service.
        //
        // This function must not call _hidl_cb(...) or return until there is no risk of blocking.
        std::unique_lock<std::mutex> lock(mPollLock, std::try_to_lock);
        if(!lock.owns_lock()){
            // cannot get the lock, hidl service will go into deadlock if it is not restarted.
            // This is guaranteed to not trigger in passthrough mode.
            LOG(ERROR) <<
                    "ISensors::poll() re-entry. I do not know what to do except killing myself.";
            ::exit(-1);
        }

        if (maxCount <= 0) {
            err = android::BAD_VALUE;
        } else {
            int bufferSize = maxCount <= kPollMaxBufferSize ? maxCount : kPollMaxBufferSize;
            data.reset(new sensors_event_t[bufferSize]);
            do {
                err = mSensorDevice->poll(
                        mSensorDevice,
                        data.get(), bufferSize);
            } while (err == -EINTR);
        }
    }

    if (err < 0) {
        _hidl_cb(ResultFromStatus(err), out, dynamicSensorsAdded);
        return Void();
    }

    const size_t count = (size_t)err;

    for (size_t i = 0; i < count; ++i) {
        if (data[i].type != SENSOR_TYPE_DYNAMIC_SENSOR_META) {
            continue;
        }

        const dynamic_sensor_meta_event_t *dyn = &data[i].dynamic_sensor_meta;

        if (!dyn->connected) {
            continue;
        }

        CHECK(dyn->sensor != nullptr);
        CHECK_EQ(dyn->sensor->handle, dyn->handle);

        SensorInfo info;
        convertFromSensor(*dyn->sensor, &info);

        size_t numDynamicSensors = dynamicSensorsAdded.size();
        dynamicSensorsAdded.resize(numDynamicSensors + 1);
        dynamicSensorsAdded[numDynamicSensors] = info;
    }

    out.resize(count);
    convertFromSensorEvents(err, data.get(), &out);

    _hidl_cb(Result::OK, out, dynamicSensorsAdded);

    return Void();
}

Return<Result> Sensors::batch(
        int32_t sensor_handle,
        int64_t sampling_period_ns,
        int64_t max_report_latency_ns) {
    return ResultFromStatus(
            mSensorDevice->setDelay(
                mSensorDevice,
                sensor_handle,
                sampling_period_ns));
}

Return<Result> Sensors::flush(int32_t sensor_handle) {
    return Result::INVALID_OPERATION;
}

Return<Result> Sensors::injectSensorData(const Event& event) {
    return Result::INVALID_OPERATION;
}

Return<void> Sensors::registerDirectChannel(
        const SharedMemInfo& mem, registerDirectChannel_cb _hidl_cb) {
    _hidl_cb(Result::INVALID_OPERATION, -1);
    return Void();
}

Return<Result> Sensors::unregisterDirectChannel(int32_t channelHandle) {
    return Result::INVALID_OPERATION;
}

Return<void> Sensors::configDirectReport(
        int32_t sensorHandle, int32_t channelHandle, RateLevel rate,
        configDirectReport_cb _hidl_cb) {
    _hidl_cb(Result::INVALID_OPERATION, -1);
    return Void();
}

// static
void Sensors::convertFromSensorEvents(
        size_t count,
        const sensors_event_t *srcArray,
        hidl_vec<Event> *dstVec) {
    for (size_t i = 0; i < count; ++i) {
        const sensors_event_t &src = srcArray[i];
        Event *dst = &(*dstVec)[i];

        convertFromSensorEvent(src, dst);
    }
}

ISensors *HIDL_FETCH_ISensors(const char * /* hal */) {
    Sensors *sensors = new Sensors;
    if (sensors->initCheck() != OK) {
        delete sensors;
        sensors = nullptr;

        return nullptr;
    }

    return sensors;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace sensors
}  // namespace hardware
}  // namespace android
